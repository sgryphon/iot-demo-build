#!/usr/bin/env pwsh

<# .SYNOPSIS
  Starts the data explorer.
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev'
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Using subscription ID $SubscriptionId"

$appName = 'iothub'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()
$decName = "dec$OrgId$Environment".ToLowerInvariant()

az kusto cluster start --name $decName -g $rgName
