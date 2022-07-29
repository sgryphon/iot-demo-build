#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy landing zone data lake into Azure.

.NOTES
  This creates date lake resources in your Azure subscription.

  See:
  * https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/scenarios/cloud-scale-analytics/best-practices/data-lake-zones 
  * https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/scenarios/cloud-scale-analytics/architectures/data-landing-zone
  * https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/scenarios/cloud-scale-analytics/tutorials/tutorial-create-data-landing-zone

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
   ./deploy-shared.ps1
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
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
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$rgName = "rg-storage-$Environment-001".ToLowerInvariant()

# Landing zone templates have Azure Monitor (but not app insights), KeyVault, and a diagnostics storage account

$stRawName = "straw$OrgId$($Environment)001".ToLowerInvariant()
$stEnrichedCuratedName = "stencur$OrgId$($Environment)001".ToLowerInvariant()
$stWorkspaceName = "stwork$OrgId$($Environment)001".ToLowerInvariant()

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{WorkloadName = 'data'; DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'IoT'; Env = $Environment }

# Create

Write-Host "Creating group $rgName"

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }
$rg = az group create -g $rgName -l $location --tags $tags | ConvertFrom-Json

# Convert tags returned from JSON result to the format used by Azure CLI create command
#$rg = az group show --name $rgName | ConvertFrom-Json
#$rgTags = $rg.tags | Get-Member -MemberType NoteProperty | ForEach-Object { "$($_.Name)=$($rg.tags.$($_.Name))" }

Write-Verbose "Creating Raw (landing, conformance) storage account $stRawName"

az storage account create --name $stRawName `
  --sku Standard_LRS `
  --allow-blob-public-access $false `
  --enable-hierarchical-namespace `
  --resource-group $rgName `
  -l $rg.location `
  --tags $tags

$lifecycleRules = @"
{
  "rules": [
    {
      "enabled": "true",
      "name": "default",
      "type": "Lifecycle",
      "definition": {
        "actions": {
          "baseBlob": {
            "tierToCool": {
              "daysAfterModificationGreaterThan": 90
            }
          },
          "snapshot": {
            "tierToCool": {
              "daysAfterCreationGreaterThan": 90
            }
          },
          "version": {
            "tierToCool": {
              "daysAfterCreationGreaterThan": 90
            }
          }
        },
        "filters": {
          "blobTypes": [
            "blockBlob"
          ],
          "prefixMatch": []
        }
      }
    }
  ]
}
"@

az storage account management-policy create --account-name $stRawName --policy ($lifecycleRules -replace '"', '""') -g $rgName

az storage fs create --name 'landing' --account-name $stRawName --auth-mode login
az storage fs create --name 'conformance' --account-name $stRawName --auth-mode login

az storage fs directory create --name 'Landing/Log' -f 'landing' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Landing/Master and Reference' -f 'landing' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Landing/Telemetry' -f 'landing' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Landing/Transactional' -f 'landing' --account-name $stRawName --auth-mode login

az storage fs directory create --name 'Conformance/Log' -f 'conformance' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Conformance/Master and Reference' -f 'conformance' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Conformance/Telemetry' -f 'conformance' --account-name $stRawName --auth-mode login
az storage fs directory create --name 'Conformance/Transactional' -f 'conformance' --account-name $stRawName --auth-mode login

Write-Verbose "Creating Enriched and Curated storage account $stEnrichedCuratedName"

az storage account create --name $stEnrichedCuratedName `
  --sku Standard_LRS `
  --allow-blob-public-access $false `
  --enable-hierarchical-namespace `
  --resource-group $rgName `
  -l $rg.location `
  --tags $tags

az storage account management-policy create --account-name $stEnrichedCuratedName --policy ($lifecycleRules -replace '"', '""') -g $rgName
  
az storage fs create --name 'standardized' --account-name $stEnrichedCuratedName --auth-mode login
az storage fs create --name 'data-products' --account-name $stEnrichedCuratedName --auth-mode login

az storage fs directory create --name 'Standardized/Log' -f 'standardized' --account-name $stEnrichedCuratedName --auth-mode login
az storage fs directory create --name 'Standardized/Master and Reference' -f 'standardized' --account-name $stEnrichedCuratedName --auth-mode login
az storage fs directory create --name 'Standardized/Telemetry' -f 'standardized' --account-name $stEnrichedCuratedName --auth-mode login
az storage fs directory create --name 'Standardized/Transactional' -f 'standardized' --account-name $stEnrichedCuratedName --auth-mode login

Write-Verbose "Creating Enriched and Curated storage account $stWorkspaceName"

az storage account create --name $stWorkspaceName `
  --sku Standard_LRS `
  --allow-blob-public-access $false `
  --enable-hierarchical-namespace `
  --resource-group $rgName `
  -l $rg.location `
  --tags $tags

az storage account management-policy create --account-name $stWorkspaceName --policy ($lifecycleRules -replace '"', '""') -g $rgName
  
az storage fs create --name 'analytics-sandbox' --account-name $stWorkspaceName --auth-mode login
az storage fs create --name 'synapse-primary-storage' --account-name $stWorkspaceName --auth-mode login

# Output

Write-Verbose "Deployment Complete"
