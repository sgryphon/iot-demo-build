#!/usr/bin/env pwsh

<# .SYNOPSIS
  Starts the server in Azure. #>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev',
    ## Numeric suffix for the server
    [int]$ServerNumber = $ENV:DEPLOY_SERVER_NUMBER ?? 1
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Using subscription ID $SubscriptionId"

$appName = 'mqtt'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()
$numericSuffix = $serverNumber.ToString("000")
$vmName = "vmmosquitto$numericSuffix"

az vm start --name $vmName -g $rgName