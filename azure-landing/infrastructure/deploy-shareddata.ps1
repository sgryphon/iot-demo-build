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

$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast'
$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
#>

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'$($AddPublicIpv4 ? ' with IPv4' : '')"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$rgName = "rg-shared-data-$Environment-001".ToLowerInvariant()

$decName = "dec$OrgId$Environment".ToLowerInvariant()

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

# Output

az kusto cluster stop --name $decName -g $rgName

Write-Verbose "Azure Data Explorer cluster is stopped for cost management; use ./start-dataexplorer.ps1 to start"

Write-Verbose "Deployment Complete"
