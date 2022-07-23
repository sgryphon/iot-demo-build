#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy server into Azure running Mosquitto MQTT server, with security.

.NOTES
  This creates a server in your Azure subscription and deploys a Mosquitto MQTT
  server, with automatic LetsEncrypt certificates using certbot.

  By default it has a public IPv6 and a DNS entry with a unique identifier based on
  your subscription prefix: "mqtt-<prefix>-dev.australiaeast.cloudapp.azure.com"

  The server uses the landing zone private network defined in `azure-landing`.

  IPv4 is also enabled by default (but can be disabled), and has the prefix:
  "mqtt-<prefix>-dev-ipv4.australiaeast.cloudapp.azure.com"

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
   ./deploy-server.ps1 -WebPassword YourSecretPassword
#>
[CmdletBinding()]
param (
    ## Authentication password for MQTT access
    [string]$MqttPassword = $ENV:DEPLOY_MQTT_PASSWORD,
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))",
    ## VM size, default is Standard_B1s
    [string]$VmSize = $ENV:DEPLOY_VM_SIZE ?? 'Standard_B2s',
    ## Linux admin account name (authentication via SSH)
    [string]$AdminUsername = $ENV:DEPLOY_ADMIN_USERNAME ?? 'iotadmin',
    ## Static server address suffix
    [string]$PrivateIpSuffix = $ENV:DEPLOY_PRIVATE_IP ?? "100c",
    ## Auto-shutdown time in UTC, default 0900 is 19:00 in Brisbane
    [string]$ShutdownUtc = $ENV:DEPLOY_SHUTDOWN_UTC ?? '0900',
    ## Email to send auto-shutdown notification to (optional)
    [string]$ShutdownEmail = $ENV:DEPLOY_SHUTDOWN_EMAIL ?? '',
    ## Add a public IPv4 (if needed; uses the last byte of the static address)
    [switch]$AddPublicIpv4 = $true
)

if (!$MqttPassword) { throw 'You must supply a value for -MqttPassword or set environment variable DEPLOY_MQTT_PASSWORD' }

<#
To run interactively, start with:

$VerbosePreference = 'Continue'

$MqttPassword = $ENV:DEPLOY_MQTT_PASSWORD
$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast'
$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
$VmSize = $ENV:DEPLOY_VM_SIZE ?? 'Standard_B2s'
$AdminUsername = $ENV:DEPLOY_ADMIN_USERNAME ?? 'iotadmin'
$ShutdownUtc = $ENV:DEPLOY_SHUTDOWN_UTC ?? '0900'
$ShutdownEmail = $ENV:DEPLOY_SHUTDOWN_UTC ?? ''
$PrivateIpSuffix = $ENV:DEPLOY_PRIVATE_IP ?? "100c"
$AddPublicIpv4 = $true
#>
$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'$($AddPublicIpv4 ? ' with IPv4' : '')"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$appName = 'mqtt'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()

$networkRgName = "rg-network-$Environment-001".ToLowerInvariant()
$vnetName = "vnet-$Environment-$Location-001".ToLowerInvariant()
$dmzSnetName = "snet-dmz-$Environment-$Location-001".ToLowerInvariant()
$dmzNsgName = "nsg-dmz-$Environment-001".ToLowerInvariant()


$vmName = 'vmmosquitto001'
$vmOsDisk = 'osdiskvmmosquitto001'
$pipDnsName = "mqtt-$OrgId-$Environment".ToLowerInvariant()
$pipName = "pip-$vmName-$Environment-$Location-001".ToLowerInvariant()
$nicName = "nic-01-$vmName-$Environment-001".ToLowerInvariant()
$ipcName = "ipc-01-$vmName-$Environment-001".ToLowerInvariant()
#$dataDiskSize = 20

$vmImage = 'UbuntuLTS'

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{ WorkloadName = 'iot'; DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'Dev'; ApplicationName = $appName; Env = $Environment }

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }


# Get network subnets, and generate addresses (this network is created by the scripts in `azure-landing`)

$dmzSnet = az network vnet subnet show --name $dmzSnetName -g $networkRgName --vnet-name $vnetName | ConvertFrom-Json

# Assumption: ends in "/64"
$dmzUlaPrefix = $dmzSnet.addressPrefixes | Where-Object { $_.StartsWith('fd') } | Select-Object -First 1
$vmIpAddress = "$($dmzUlaPrefix.Substring(0, $dmzUlaPrefix.Length - 3))$PrivateIpSuffix"

# Assumption: ends in "0/24"
$dmzIPv4Prefix = $dmzSnet.addressPrefixes | Where-Object { $_.StartsWith('10.') } | Select-Object -First 1
$privateIPv4Suffix = [int]"0x$($PrivateIpSuffix.Substring($PrivateIpSuffix.Length -2))"
$vmIPv4 = "$($dmzIPv4Prefix.Substring(0, $dmzIPv4Prefix.Length - 4))$privateIPv4Suffix"


# Create

Write-Verbose "Adding Network security group rule 'AllowMQTT' for port 5683, 5684 to $dmzNsgName"
az network nsg rule create --name AllowMQTT `
                           --nsg-name $dmzNsgName `
                           --priority 2200 `
                           --resource-group $networkRgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --source-port-ranges "*" `
                           --direction Inbound `
                           --protocol Udp `
                           --destination-port-ranges 8883 8083

Write-Verbose "Creating resource group $rgName"
az group create --name $rgName -l $Location --tags $tags

Write-Verbose "Creating Public IP addresses $pipName (DNS $pipDnsName)"
az network public-ip create `
  --name $pipName  `
  --dns-name $pipDnsName `
  --resource-group $rgName `
  --location $Location `
  --sku Standard  `
  --allocation-method static  `
  --version IPv6 `
  --tags $tags

# Azure only supports dual stack; primary NIC IP config must be IPv4

Write-Verbose "Creating Network interface controller $nicName (required IPv4 $vmIPv4)"
az network nic create `
  --name $nicName `
  --resource-group $rgName `
  --subnet $dmzSnet.Id `
  --private-ip-address $vmIPv4 `
  --tags $tags

Write-Verbose "Adding NIC IP Config $ipcName ($vmIpAddress, $pipName) to $nicName"
az network nic ip-config create `
  --name $ipcName `
  --nic-name $nicName  `
  --resource-group $rgName `
  --subnet $dmzSnet.Id `
  --private-ip-address $vmIpAddress `
  --private-ip-address-version IPv6 `
  --public-ip-address $pipName

$hostName = $(az network public-ip show --name $pipName --resource-group $rgName --query dnsSettings.fqdn --output tsv)

if ($AddPublicIpv4) {
  $pipv4Name = "pipv4-$vmName-$Environment-$Location-001".ToLowerInvariant()
  $pipv4DnsName = "mqtt-$OrgId-$Environment-ipv4".ToLowerInvariant()

  Write-Verbose "Creating Public IPv4 addresses $pipv4Name (DNS $pipv4DnsName)"
  az network public-ip create `
    --name $pipv4Name  `
    --dns-name $pipv4DnsName `
    --resource-group $rgName `
    --location $Location  `
    --sku Standard  `
    --allocation-method static  `
    --version IPv4 `
    --tags $tags

  # the auto-created config name is ipconfig1
  az network nic ip-config update `
    --name 'ipconfig1' `
    --nic-name $nicName `
    -g $rgName `
    --public-ip-address $pipv4Name

  $hostNameIpv4 = $(az network public-ip show --name $pipv4Name --resource-group $rgName --query dnsSettings.fqdn --output tsv)
}
   
Write-Verbose "Configurating cloud-init.txt~ file with host names: $hostNames"
((((Get-Content -Path (Join-Path $PSScriptRoot cloud-init.txt) -Raw) `
  -replace 'INIT_HOST_NAME',$hostName) `
  -replace 'INIT_HOST_NAME_IPV4',$hostNameIpv4) `
  -replace 'INIT_PASSWORD_INPUT',$WebPassword) `
  | Set-Content -Path (Join-Path $PSScriptRoot cloud-init.txt~)

Write-Verbose "Creating Virtual machine $vmName (size $vmSize)"
az vm create `
    --resource-group $rgName `
    --name $vmName `
    --size $VmSize `
    --image $vmImage `
    --os-disk-name $vmOsDisk `
    --admin-username $AdminUsername `
    --generate-ssh-keys `
    --nics $nicName `
    --public-ip-sku Standard `
    --custom-data cloud-init.txt~ `
    --tags $tags

#    --data-disk-caching None `
#    --data-disk-sizes-gb $DataDiskSize `

if ($ShutdownUtc) {
  if ($ShutdownEmail) {
    az vm auto-shutdown -g $rgName -n $vmName --time $ShutdownUtc --email $ShutdownEmail
  } else {
    az vm auto-shutdown -g $rgName -n $vmName --time $ShutdownUtc
  }
}

Write-Verbose "Virtual machine created"

$vm = (az vm show --name $vmName -g $rgName -d) | ConvertFrom-Json
$vm | Format-List name, fqdns, publicIps, privateIps, location, hardwareProfile

Write-Verbose "Deployment Complete"
# ssh iotadmin@mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
