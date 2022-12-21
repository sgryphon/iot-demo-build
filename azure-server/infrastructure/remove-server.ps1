#!/usr/bin/env pwsh

<# .SYNOPSIS
  Remove the Azure development infrastructure resource group. #>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev'
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Removing from context subscription ID $SubscriptionId"

$appName = 'web'

$rgName = "rg-$appName-$Environment-01".ToLowerInvariant()

az group delete --name $rgName