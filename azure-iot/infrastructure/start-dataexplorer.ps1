#!/usr/bin/env pwsh

<# .SYNOPSIS
  Starts the data explorer.
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
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()
$decName = "dec$OrgId$Environment".ToLowerInvariant()

az kusto cluster start --name $decName -g $rgName
