#!/usr/bin/env pwsh

<# .SYNOPSIS
  Starts the server in Azure. #>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev'
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Using subscription ID $SubscriptionId"

$appName = 'mqtt'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()
$vmName = 'vmmosquitto001'

az vm start --name $vmName -g $rgName