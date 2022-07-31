#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy landing zone shared data services into Azure.

.NOTES
  This creates shared data services in your Azure subscription.

  This includes Azure Data Explorer and Azure Synapse.

  Running these scripts requires the following to be installed:
  * PowerShell, https://github.com/PowerShell/PowerShell
  * Azure CLI, https://docs.microsoft.com/en-us/cli/azure/

  You also need to connect to Azure (log in), and set the desired subscription context.

  Follow standard naming conventions from Azure Cloud Adoption Framework, 
  with an additional organisation or subscription identifier (after app name) in global names 
  to make them unique.
  https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming

  Follow standard tagging conventions from  Azure Cloud Adoption Framework.
  https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

.EXAMPLE

   az login
   az account set --subscription <subscription id>
   $VerbosePreference = 'Continue'
   ./deploy-shareddata.ps1
#>
[CmdletBinding()]
param (
    ## Authentication password for MQTT access
    [string]$SynapseSqlPassword = $ENV:DEPLOY_SYNAPSE_SQL_PASSWORD,
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))",
    ## Data Explorer cluster SKU name (default 'Dev(No SLA)_Standard_E2a_v4')
    [string]$DecSkuName = $ENV:DEPLOY_DEC_SKU_NAME ?? "Dev(No SLA)_Standard_E2a_v4",
    ## Data Explorer cluster SKU name (default 'Developer')
    [string]$DecSkuTier = $ENV:DEPLOY_DEC_SKU_TIER ?? "Basic"
)

<#
To run interactively, start with:

$VerbosePreference = 'Continue'

$SynapseSqlPassword = ...

$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast'
$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
$DecSkuName = $ENV:DEPLOY_DEC_SKU_NAME ?? "Dev(No SLA)_Standard_E2a_v4"
$DecSkuTier = $ENV:DEPLOY_DEC_SKU_TIER ?? "Basic"

#>

if (!$SynapseSqlPassword) { throw 'You must supply a value for -SynapseSqlPassword or set environment variable DEPLOY_SYNAPSE_SQL_PASSWORD' }

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'$($AddPublicIpv4 ? ' with IPv4' : '')"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$rgName = "rg-shared-data-$Environment-001".ToLowerInvariant()

$decName = "dec$OrgId$Environment".ToLowerInvariant()

$synWorkspaceName = "syn-$OrgId-$Environment".ToLowerInvariant()
$stWorkspaceName = "stwork$OrgId$($Environment)001".ToLowerInvariant()
$synSqlAdmin = "synadmin"

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{ WorkloadName = 'data'; DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'IoT'; Env = $Environment }

# Create

Write-Host "Creating group $rgName"

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }
$rg = az group create -g $rgName -l $location --tags $tags | ConvertFrom-Json

# Convert tags returned from JSON result to the format used by Azure CLI create command
#$rg = az group show --name $rgName | ConvertFrom-Json
#$rgTags = $rg.tags | Get-Member -MemberType NoteProperty | ForEach-Object { "$($_.Name)=$($rg.tags.$($_.Name))" }

Write-Verbose "Create Azure Data Explorer cluster $decName"

az extension add -n kusto

az kusto cluster create `
  --resource-group $rgName `
  -l $rg.location `
  --name $decName `
  --sku name=$DecSkuName tier=$DecSkuTier

Write-Verbose "Stopping Azure Data Explorer cluster for cost management; use ./start-dataexplorer.ps1 to start"

az kusto cluster stop --name $decName -g $rgName

  
Write-Verbose "Create Synapse Analytics workspace $synWorkspaceName"

# https://docs.microsoft.com/en-us/azure/synapse-analytics/quickstart-create-workspace-cli

az provider register --namespace Microsoft.Sql
# Note: may take a while to register
Start-Sleep -Seconds 10

az synapse workspace create `
  --name $synWorkspaceName `
  --resource-group $rgName `
  --storage-account $stWorkspaceName `
  --file-system 'synapse-primary-storage' `
  --sql-admin-login-user $synSqlAdmin `
  --sql-admin-login-password $SynapseSqlPassword `
  --location $Location `
  --tags $tags

$workspace = az synapse workspace show --name $synWorkspaceName --resource-group $rgName | ConvertFrom-Json

$connectivityResult = Invoke-WebRequest -Uri $workspace.connectivityEndpoints.dev -Headers @{ "Accept" = "application/json" } -SkipHttpErrorCheck
if ($connectivityResult.StatusCode -eq 403) {
  $connectivityContent = $connectivityResult.Content | ConvertFrom-Json
  if ($connectivityContent.message.StartsWith('Client Ip address : ')) {
    $clientIp = $connectivityContent.message.Substring(20)
    Write-Verbose "Creating a firewall rule to enable access for IP address: $clientIp"

    az synapse workspace firewall-rule create `
      --end-ip-address $clientIp `
      --start-ip-address $clientIp `
      --name "Allow Client IP" `
      --resource-group $rgName `
      --workspace-name $synWorkspaceName
  } else {
    Write-Warning("Unexpected message: $($connectivityContent.message)")
  }
}

Write-Verbose "Create Synapse linked service to raw storage account"

az synapse linked-service create --workspace-name $synWorkspaceName --name AzureDataLakeRaw001 --file '@"data/AzureDataLakeRaw001.json"'

Write-Verbose "Create Synapse linked service to enriched and curated storage account"

az synapse linked-service create --workspace-name $synWorkspaceName --name AzureDataLakeEnrichedCurated001 --file '@"data/AzureDataLakeEnrichedCurated001.json"'

# Output

Write-Verbose "Synapse workspace: $($workspace.connectivityEndpoints.web)"

Write-Verbose "Deployment Complete"
