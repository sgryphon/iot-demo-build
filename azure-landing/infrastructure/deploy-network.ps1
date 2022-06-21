#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy landing zone network into Azure.

.NOTES
  This creates a core network in your Azure subscription.

  The network is dual stack and you should pass in -UlaGlobalId and subnet IDs
  for the IPv6 Unique Local Address network, or if you do not they will be 
  generated based on a consistent unique hash of the subscription ID.
  For more information on ULAs see https://en.wikipedia.org/wiki/Unique_local_address

  IPv4 addresses use the first byte of the ULA global ID, and the last byte of the
  subnet ID to generate a 10.x.0.0/16 network and 10.x.y.0/24 subnets.

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
   ./deploy-network.ps1 -WebPassword YourSecretPassword
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## The Azure region where the resource is deployed.
    [string]$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast',
    ## IPv6 Unique Local Address GlobalID to use (default hash of subscription ID)
    [string]$UlaGlobalId = $ENV:DEPLOY_GLOBAL_ID ?? (Get-FileHash -InputStream ([IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes((az account show --query id --output tsv))))).Hash.Substring(0, 10),
    ## IPv6 Unique Local Address SubnetID to use (default 101)
    [string]$UlaCoreSubnetId = $ENV:DEPLOY_CORE_SUBNET_ID ?? ("0101"),
    ## IPv6 Unique Local Address SubnetID to use (default 102)
    [string]$UlaDmzSubnetId = $ENV:DEPLOY_DMZ_SUBNET_ID ?? ("0102")
)

<#
To run interactively, start with:

$VerbosePreference = 'Continue'

$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
$Location = $ENV:DEPLOY_LOCATION ?? 'australiaeast'
$UlaGlobalId = $ENV:DEPLOY_GLOBAL_ID ?? (Get-FileHash -InputStream ([IO.MemoryStream]::new([Text.Encoding]::UTF8.GetBytes((az account show --query id --output tsv))))).Hash.Substring(0, 10),
$UlaCoreSubnetId = $ENV:DEPLOY_CORE_SUBNET_ID ?? ("0101"),
$UlaDmzSubnetId = $ENV:DEPLOY_DMZ_SUBNET_ID ?? ("0102")
#>

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Deploying scripts for environment '$Environment' in subscription '$SubscriptionId'$($AddPublicIpv4 ? ' with IPv4' : '')"

# Following standard naming conventions from Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-naming
# With an additional organisation or subscription identifier (after app name) in global names to make them unique 

$rgName = "rg-network-$Environment-001".ToLowerInvariant()

# Landing zone templates have a VNet RG, with one network, and four subnets:
# GatewaySubnet (.0/26), AzureFirewallSubnet (.64/26),
# JumpboxSubnet (.128/26) - with Jumpbox-NSG (allow inbound vnet-vnet, loadbal-any; outbound vnet-vnet, any-internet),
# CoreSubnet (.4.0/22) - with Core-NSG (allow inbound vnet-vnet, loadbal-any; outbound vnet-vnet, any-internet)

# For this script:
#  Core is internal network
#  DMZ has Internet access
# Additional subnets to be added as needed

$vnetName = "vnet-$Environment-$Location-001".ToLowerInvariant()
$coreSnetName = "snet-core-$Environment-$Location-001".ToLowerInvariant()
$coreNsgName = "nsg-core-$Environment-001".ToLowerInvariant()
$dmzSnetName = "snet-dmz-$Environment-$Location-001".ToLowerInvariant()
$dmzNsgName = "nsg-dmz-$Environment-001".ToLowerInvariant()

# Global will default to unique value per subscription
$prefix = "fd$($UlaGlobalId.Substring(0, 2)):$($UlaGlobalId.Substring(2, 4)):$($UlaGlobalId.Substring(6))"
$globalAddress = [IPAddress]"$($prefix)::"
$coreSubnetAddress = [IPAddress]"$($prefix):$UlaCoreSubnetId::"
$dmzSubnetAddress = [IPAddress]"$($prefix):$UlaDmzSubnetId::"
$vnetIpPrefix = "$globalAddress/48"
$coreSnetIpPrefix = "$coreSubnetAddress/64"
$dmzSnetIpPrefix = "$dmzSubnetAddress/64"

# Azure only supports dual-stack (not single stack IPv6)
# "At least one IPv4 ipConfiguration is required for an IPv6 ipConfiguration on the network interface"

# Use the first byte of the ULA Global ID, and the last byte of the subnet ID
$prefixByte = [int]"0x$($UlaGlobalId.Substring(0, 2))"
$vnetIPv4 = "10.$prefixByte.0.0/16"
$coreSubnet = [int]"0x$UlaCoreSubnetId"
$coreSnetIPv4 = "10.$prefixByte.$($coreSubnet -bAnd 0xFF).0/24"
$dmzSubnet = [int]"0x$UlaDmzSubnetId"
$dmzSnetIPv4 = "10.$prefixByte.$($dmzSubnet -bAnd 0xFF).0/24"

# Following standard tagging conventions from  Azure Cloud Adoption Framework
# https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/azure-best-practices/resource-tagging

$TagDictionary = @{ DataClassification = 'Non-business'; Criticality = 'Low';
  BusinessUnit = 'IoT'; Env = $Environment }

# Convert dictionary to tags format used by Azure CLI create command
$tags = $TagDictionary.Keys | ForEach-Object { $key = $_; "$key=$($TagDictionary[$key])" }

# Create

Write-Verbose "Creating resource group $rgName"
az group create --name $rgName -l $Location --tags $tags

Write-Verbose "Creating core network security group $coreNagName"
az network nsg create --name $coreNsgName -g $rgName -l $Location --tags $tags

Write-Verbose "Creating DMZ network security group $dmzNsgName"
az network nsg create --name $dmzNsgName -g $rgName -l $Location --tags $tags

Write-Verbose "Adding Network security group rule 'AllowSSH' for port 22 to $dmzNsgName"
az network nsg rule create --name AllowSSH `
                           --nsg-name $dmzNsgName `
                           --priority 1000 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --source-port-ranges "*" `
                           --direction Inbound `
                           --destination-port-ranges 22

Write-Verbose "Adding Network security group rule 'AllowICMP' for ICMP to $dmzNsgName"
az network nsg rule create --name AllowICMP `
                           --nsg-name $dmzNsgName `
                           --priority 1001 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --direction Inbound `
                           --destination-port-ranges "*" `
                           --protocol Icmp

Write-Verbose "Adding Network security group rule 'AllowHTTP' for port 80, 443 to $dmzNsgName"
az network nsg rule create --name AllowHTTP `
                           --nsg-name $dmzNsgName `
                           --priority 1002 `
                           --resource-group $rgName `
                           --access Allow `
                           --source-address-prefixes "*" `
                           --source-port-ranges "*" `
                           --direction Inbound `
                           --destination-port-ranges 80 443

# Check rules
# az network nsg rule list --nsg-name $nsgDmzName --resource-group $rgName

Write-Verbose "Creating virtual network $vnetName ($vnetIpPrefix, $vnetIPv4)"
az network vnet create --name $vnetName `
                       --resource-group $rgName `
                       --address-prefixes $vnetIpPrefix $vnetIPv4 `
                       --location $Location `
                       --tags $tags

Write-Verbose "Creating core subnet $coreSnetName ($coreSnetIpPrefix, $coreSnetIPv4)"
az network vnet subnet create --name $coreSnetName `
                              --address-prefix $coreSnetIpPrefix $coreSnetIPv4 `
                              --resource-group $rgName `
                              --vnet-name $vnetName `
                              --network-security-group $coreNsgName

Write-Verbose "Creating DMZ subnet $dmzSnetName ($dmzSnetIpPrefix, $dmzSnetIPv4)"
az network vnet subnet create --name $dmzSnetName `
                              --address-prefix $dmzSnetIpPrefix $dmzSnetIPv4 `
                              --resource-group $rgName `
                              --vnet-name $vnetName `
                              --network-security-group $dmzNsgName

Write-Verbose "Deployment Complete"
