#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy server into Azure for.

.NOTES
  This creates a server in your Azure subscription.

  By default it has a public IPv6 and a DNS entry with a unique identifier based on
  your subscription prefix: "lwm2m-<prefix>-dev.australiaeast.cloudapp.azure.com"

  The internal network will have a randomly allocation IPv6 Unique Local Address
  dual stack network (or you can pass in -UlaGlobalId or -UlaSubnetId for specific values).
  For more information on ULAs see https://en.wikipedia.org/wiki/Unique_local_address

  You can also add a public IPv4 if needed via the flag -AddPublicIpv4

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
   ./deploy-server.ps1 -AddPublicIpv4
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))",
    ## VM size, default is Standard_B1s
    [string]$VmSize = $ENV:DEPLOY_VM_SIZE ?? 'Standard_B2s',
    [string]$AdminUsername = $ENV:DEPLOY_ADMIN_USERNAME ?? 'iotadmin',
    #[string]$AdminPassword = $ENV:DEPLOY_ADMIN_PASSWORD ?? 'iotPassword01',
    ## IPv6 Unique Local Address GlobalID to use (default random)
    [string]$UlaGlobalId = $ENV:DEPLOY_GLOBAL_D ?? ("{0:x2}{1:x8}" -f (Get-Random -Max 0xFF), (Get-Random)),
    ## IPv6 Unique Local Address SubnetID to use (default random)
    [string]$UlaSubnetId = $ENV:DEPLOY_GLOBAL_D ?? ("{0:x4}" -f (Get-Random -Max 0xFFFF)),
    ## Auto-shutdown time in UTC, default 0900 is 19:00 in Brisbane
    [string]$ShutdownUtc = $ENV:DEPLOY_SHUTDOWN_UTC ?? '0900',
    ## Email to send auto-shutdown notification to (optional)
    [string]$ShutdownEmail = $ENV:DEPLOY_SHUTDOWN_EMAIL ?? '',
    ## Add a public IPv4 (if needed)
    [switch]$AddPublicIpv4
)

<#
To run interactively, start with:

$VerbosePreference = 'Continue'

$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast'
$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
$VmSize = $ENV:DEPLOY_VM_SIZE ?? 'Standard_B2s'
$AdminUsername = $ENV:DEPLOY_ADMIN_USERNAME ?? 'iotadmin'
$UlaGlobalId = $ENV:DEPLOY_GLOBAL_D ?? ("{0:x2}{1:x8}" -f (Get-Random -Max 0xFF), (Get-Random))
$UlaSubnetId = $ENV:DEPLOY_GLOBAL_D ?? ("{0:x4}" -f (Get-Random -Max 0xFFFF))
$ShutdownUtc = $ENV:DEPLOY_SHUTDOWN_UTC ?? '0900'
$ShutdownEmail = $ENV:DEPLOY_SHUTDOWN_UTC ?? ''
$AddPublicIpv4 = $true
#>
$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$appName = 'iotdemo'
$rgName = "rg-$appName-$Environment-001".ToLowerInvariant()

$nsgName = "nsg-lwm2m-$Environment-001".ToLowerInvariant()
$vnetName = "vnet-$Environment-$Location-001".ToLowerInvariant()
$snetName = "snet-$Environment-$Location-001".ToLowerInvariant()

$prefix = "fd$($UlaGlobalId.Substring(0, 2)):$($UlaGlobalId.Substring(2, 4)):$($UlaGlobalId.Substring(6))"
$globalAddress = [IPAddress]"$($prefix)::"
$subnetAddress = [IPAddress]"$($prefix):$UlaSubnetId::"
$vnetIpPrefix = "$globalAddress/48"
$snetIpPrefix = "$subnetAddress/64"

$vmName = 'vmlwm2m001'
$vmOsDisk = 'osdiskvmlwm2m001'
$pipDnsName = "lwm2m-$OrgId-$Environment".ToLowerInvariant()
$pipName = "pip-$vmName-$Environment-$Location-001".ToLowerInvariant()
$nicName = "nic-01-$vmName-$Environment-001".ToLowerInvariant()
$ipcName = "ipc-01-$vmName-$Environment-001".ToLowerInvariant()
#$dataDiskSize = 20

$vmImage = 'UbuntuLTS'
$vmIpAddress = "$($subnetAddress)d"

# Azure only supports dual-stack (not single stack IPv6)
# "At least one IPv4 ipConfiguration is required for an IPv6 ipConfiguration on the network interface"

$vnetIPv4 = '10.0.0.0/8'
$subnet = [int]"0x$UlaSubnetId"
$snetIPv4 = "10.$($subnet -shr 8).$($subnet -bAnd 0xFF).0/24"
$vmIPv4 = "10.$($subnet -shr 8).$($subnet -bAnd 0xFF).13"

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{ WorkloadName = 'iot'; DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'Dev'; ApplicationName = $appName; Env = $Environment }

# Create

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }

Write-Verbose "Creating resource group $rgName"
az group create --name $rgName -l $Location --tags $tags

Write-Verbose "Creating Network security group $nsgName"
az network nsg create --name $nsgName -g $rgName -l $Location --tags $tags

Write-Verbose "Adding Network security group rule 'AllowSSH' for port 22 to $nsgName"
az network nsg rule create --name AllowRDP `
                           --nsg-name $nsgName `
                           --priority 1000 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --source-port-ranges "*" `
                           --direction Inbound `
                           --destination-port-ranges 22

Write-Verbose "Adding Network security group rule 'AllowICMP' for ICMP to $nsgName"
az network nsg rule create --name AllowICMP `
                           --nsg-name $nsgName `
                           --priority 1001 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --direction Inbound `
                           --destination-port-ranges "*" `
                           --protocol Icmp

Write-Verbose "Adding Network security group rule 'AllowHTTP' for port 80, 443 to $nsgName"
az network nsg rule create --name AllowLDAP `
                           --nsg-name $nsgName `
                           --priority 1002 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --source-port-ranges "*" `
                           --direction Inbound `
                           --destination-port-ranges 80 443

Write-Verbose "Creating Virtual network $vnetName ($vnetIpPrefix, $vnetIPv4)"
az network vnet create --name $vnetName `
                       --resource-group $rgName `
                       --address-prefixes $vnetIpPrefix $vnetIPv4 `
                       --location $Location `
                       --tags $tags

Write-Verbose "Creating Subnet $snetName ($snetIpPrefix, $snetIPv4)"
az network vnet subnet create --name $snetName `
                              --address-prefix $snetIpPrefix $snetIPv4 `
                              --resource-group $rgName `
                              --vnet-name $vnetName `
                              --network-security-group $nsgName

Write-Verbose "Creating Public IP addresses $pipName (DNS $pipDnsName)"
az network public-ip create `
  --name $pipName  `
  --dns-name $pipDnsName `
  --resource-group $rgName  `
  --location $Location `
  --sku Standard  `
  --allocation-method static  `
  --version IPv6 `
  --tags $tags

<#
#>

# Azure only supports dual stack; primary NIC IP config must be IPv4

Write-Verbose "Creating Network interface controller $nicName (required IPv4 $vmIPv4)"
az network nic create `
  --name $nicName `
  --resource-group $rgName `
  --network-security-group $nsgName `
  --vnet-name $vnetName `
  --subnet $snetName `
  --private-ip-address $vmIPv4 `
  --tags $tags

#--public-ip-address $pipv4Name `

Write-Verbose "Adding NIC IP Config $ipcName ($vmIpAddress, $pipName) to $nicName"
az network nic ip-config create `
  --name $ipcName `
  --nic-name $nicName  `
  --resource-group $rgName `
  --vnet-name $vnetName `
  --subnet $snetName `
  --private-ip-address $vmIpAddress `
  --private-ip-address-version IPv6 `
  --public-ip-address $pipName

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
    --custom-data cloud-init.txt `
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

if ($AddPublicIpv4) {
  $pipv4Name = "pipv4-$vmName-$Environment-$Location-001".ToLowerInvariant()
  Write-Verbose "Creating Public IPv4 addresses $pipv4Name (DNS $pipDnsName)"
  az network public-ip create `
    --name $pipv4Name  `
    --dns-name $pipDnsName `
    --resource-group $rgName  `
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
}

Write-Verbose "Opening port 80 for $vmName"
az vm open-port --port 80 -g $rgName --name $vmName

Write-Verbose "Virtual machine created"

$vm = (az vm show --name $vmName -g $rgName -d) | ConvertFrom-Json
$vm | Format-List name, fqdns, publicIps, privateIps, location, hardwareProfile

Write-Verbose "Deployment Complete"
# ssh iotadmin@lwm2m-0xacc5-dev.australiaeast.cloudapp.azure.com
