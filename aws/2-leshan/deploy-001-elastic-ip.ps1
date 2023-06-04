#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy public IP address to use for the Leshan server

.NOTES
  Running these scripts requires the following to be installed:
  * PowerShell, https://github.com/PowerShell/PowerShell
  * AWS CLI, https://docs.aws.amazon.com/cli/index.html

  You also need credetials to connect to the target AWS account.

.EXAMPLE

  aws configure sso
  $VerbosePreference = 'Continue'
  ./deploy-001-landing-vpc.ps1
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev',
    ## Static server address suffix
    [string]$PrivateIpSuffix = $ENV:DEPLOY_PRIVATE_IP ?? "100d"
)

<#
To run interactively, start with:

$VerbosePreference = 'Continue'

$ENV:AWS_DEFAULT_PROFILE="AWSAdministratorAccess-632781653573"
$ENV:AWS_DEFAULT_REGION="ap-southeast-2"

$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
#>

$ErrorActionPreference="Stop"

$stackTemplateFile = Join-Path $PSScriptRoot "cf-001-elastic-ip.yaml"
$stackName = "leshan-ip-${Environment}".ToLowerInvariant()

$networkStackName = "core-network-${Environment}".ToLowerInvariant()
$vpcName = "$networkStackName-01".ToLowerInvariant()
$publicSubnet02Name = "$networkStackName-public-subnet-02".ToLowerInvariant()

# Check if profile and region environment variables are set
if (-not $ENV:AWS_PROFILE) {
  Write-Host 'You need to configure the default profile, e.g. $ENV:AWS_PROFILE="AWSAdministratorAccess-632781653573"'
  throw 'You need to configure the default profile, e.g. $ENV:AWS_PROFILE="AWSAdministratorAccess-632781653573"'
}
if (-not $ENV:AWS_DEFAULT_REGION) {
  Write-Host 'You need to configure the default region, e.g. $ENV:AWS_DEFAULT_REGION="ap-southeast-2"'
  throw 'You need to configure the default region, e.g. $ENV:AWS_DEFAULT_REGION="ap-southeast-2"'
}
  
# Getting the network stack

Write-Verbose "Getting VPC $vpcName"
$vpc = aws ec2 describe-vpcs `
    --filters "Name=tag:Name,Values=$vpcName" `
    | ConvertFrom-Json `
    | Select-Object -ExpandProperty Vpcs `
    | Select-Object -First 1

Write-Verbose "Getting public dual stack subnet $publicSubnet02Name" 
$dualStackSubnet = aws ec2 describe-subnets `
  --filters "Name=tag:Name,Values=$publicSubnet02Name" "Name=vpc-id,Values=$($vpc.VpcId)" `
  | ConvertFrom-Json `
  | Select-Object -ExpandProperty Subnets `
  | Select-Object -First 1   
 
# Assumption: ends in "::/64", and we want to keep the "::" part
$dsIpv6Prefix = $dualStackSubnet.Ipv6CidrBlockAssociationSet[0].Ipv6CidrBlock
$dsIpv6Address = "$($dsIpv6Prefix.Substring(0, $dsIpv6Prefix.Length - 3))$PrivateIpSuffix"

# Assumption: ends in "0/24"
$dsIpv4Prefix = $dualStackSubnet.CidrBlock
$privateIpv4Suffix = [int]"0x$($PrivateIpSuffix.Substring($PrivateIpSuffix.Length -2))"
$dsIpv4Address = "$($dsIpv4Prefix.Substring(0, $dsIpv4Prefix.Length - 4))$privateIpv4Suffix"

# Creating Elastic IP

Write-Verbose "Deploying cloud formation to create EIP stack $stackName"

aws cloudformation deploy `
    --template-file $stackTemplateFile `
    --stack-name $stackName `
    --tags `
      "BusinessUnit=IoT" `
      "DataClassification=Non-business" `
      "Environment=$Environment"
if (!$?) { throw "AWS CLI returned error: $LastExitCode" }

# aws cloudformation delete-stack --stack-name 'leshan-ip-dev'

# Retrieve the output values
$stackOutputs = aws cloudformation describe-stacks `
    --stack-name $stackName `
    | ConvertFrom-Json `
    | Select-Object -ExpandProperty Stacks

Write-Host "Stack outputs:`n$($stackOutputs | ConvertTo-Json -Depth 10)"

$eipAddressName = "$stackName-address"
$eipAddress = aws cloudformation list-exports --query "Exports[?Name=='$eipAddressName'].Value" --output text
$eipAllocationIdName = "$stackName-allocation-id"
$eipAllocationId = aws cloudformation list-exports --query "Exports[?Name=='$eipAllocationIdName'].Value" --output text

Write-Host "`nAssign IPv4 DNS (based on EIP $eipAllocationId)" -ForegroundColor Green
Write-Host "IPv4 address: $eipAddress" 

Write-Host "`nAssign IPv6 DNS (based on subnet $publicSubnet02Name)" -ForegroundColor Green
Write-Host "IPv6 subnet: $dsIpv6Prefix" 
Write-Host "IPv6 address: $dsIpv6Address" 
Write-Host "Internal private IPv4 address: $dsIpv4Address" 
