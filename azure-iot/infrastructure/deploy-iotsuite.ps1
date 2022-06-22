#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy IoT into Azure.

.NOTES
  This creates IoT Hub, and Device Provisioning Services.

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
   ./deploy-iot.ps1
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))",
    ## IoT Hub SKU, default is F1
    [string]$IoTHubSku = $ENV:DEPLOY_IOTHUB_SKU ?? 'F1',
    ## DPS SKU (currently only value is S1)
    [string]$DpsSku = $ENV:DEPLOY_DPS_SKU ?? 'S1'
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

$appName = 'iotsuite'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()

# Landing zone templates have Azure Monitor (but not app insights), KeyVault, and a diagnostics storage account

$iotName = "iot-$appName-$OrgId-$Environment".ToLowerInvariant()
$dpsName = "dps-$appName-$OrgId-$Environment".ToLowerInvariant()

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{ WorkloadName = 'iot'; DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'IoT'; ApplicationName = $appName; Env = $Environment }


# Create

Write-Host "Creating group $rgName"

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }
$rg = az group create -g $rgName -l $location --tags $tags | ConvertFrom-Json

# Convert tags returned from JSON result to the format used by Azure CLI create command
#$rg = az group show --name $rgName | ConvertFrom-Json
#$rgTags = $rg.tags | Get-Member -MemberType NoteProperty | ForEach-Object { "$($_.Name)=$($rg.tags.$($_.Name))" }

# IoT Central has: IoT Hub, DPS, Stream Analytics, Data Explorer, SQL DB, Cosmos DB (https://docs.microsoft.com/en-us/azure/iot-central/core/concepts-architecture)
# az iot central app create -n $iotcName -g $rgName -s $iotcSubdomain --sku ST0 --display-name $iotcDisplayName
# TTN integration uses Event Hub, App Service Plan, and Storage (function app)

Write-Verbose "Creating Device Provisioning Service $dpsName"

az iot dps create `
  --resource-group $rgName `
  -l $rg.location `
  --name $dpsName `
  --sku  $dpsSku `
  --tags $tags


Write-Verbose "Creating IoT hub $iotName"

az iot hub create `
  --resource-group $rgName `
  -l $rg.location `
  --name $iotName `
  --sku $iotHubSku `
  --partition-count 2 `
  --tags $tags

Write-Verbose "Linking IoT hub $iotName to DPS $dpsName"

az iot dps linked-hub create `
  -g $rgName `
  --dps-name $dpsName `
  --hub-name $iotName


# Azure Digital Twins

# Output

#Write-Verbose "Client App Instrumentation Key: $aiKey"

Write-Verbose "Deployment Complete"
