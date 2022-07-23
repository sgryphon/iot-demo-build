#!/usr/bin/env pwsh

<# .SYNOPSIS
  Imports the factory ontology model
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Using subscription ID $SubscriptionId"

$appName = 'iotcore'
$dtName = "dt-$appName-$OrgId-$Environment".ToLowerInvariant()

Write-Verbose "Importing factory ontology models to $dtName"

Write-Verbose "Geo ontology models"

az dt model create --dt-name $dtName --from-directory (Join-Path $PSScriptRoot 'Ontology/Geo') 

Write-Verbose "SAREF Core ontology models"

az dt model create --dt-name $dtName --from-directory (Join-Path $PSScriptRoot 'Ontology/Core') 

Write-Verbose "SAREF Building ontology models"

az dt model create --dt-name $dtName --from-directory (Join-Path $PSScriptRoot 'Ontology/Building') 

Write-Verbose "SAREF Manufacturing ontology models"

az dt model create --dt-name $dtName --from-directory (Join-Path $PSScriptRoot 'Ontology/Manufacturing') 

Write-Verbose "Chocolate factory ontology models"

az dt model create --dt-name $dtName --from-directory (Join-Path $PSScriptRoot 'Ontology/Factory') 
