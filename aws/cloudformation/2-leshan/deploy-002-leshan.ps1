#!/usr/bin/env pwsh

<# .SYNOPSIS
  Deploy landing virtual private cloud (vpc) network.

.NOTES
  This creates a core network in your AWS account

  The network is dual stack and you should pass in -UlaGlobalId and subnet IDs
  for the IPv6 Unique Local Address network, or if you do not they will be 
  generated based on a consistent unique hash of the subscription ID.
  For more information on ULAs see https://en.wikipedia.org/wiki/Unique_local_address

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

$stackTemplateFile = Join-Path $PSScriptRoot "cf-002-leshan.yaml"
$stackName = "leshan-${Environment}".ToLowerInvariant()

$networkStackName = "core-network-${Environment}".ToLowerInvariant()
$vpcName = "$networkStackName-01".ToLowerInvariant()
$publicSubnet01Name = "$networkStackName-public-subnet-01".ToLowerInvariant()
$publicSubnet02Name = "$networkStackName-public-subnet-02".ToLowerInvariant()

$keyName = "leshan-${Environment}-key".ToLowerInvariant()
$sshFolder = "~/.ssh"

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

Write-Verbose "Getting public IPv6 subnet $publicSubnet01Name" 
$ipv6Subnet = aws ec2 describe-subnets `
  --filters "Name=tag:Name,Values=$publicSubnet01Name" "Name=vpc-id,Values=$($vpc.VpcId)" `
  | ConvertFrom-Json `
  | Select-Object -ExpandProperty Subnets `
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

Write-Verbose "Using addresses $dsIpv6Address, $dsIpv4Address" 

$keyPath ="$sshFolder/$keyName.pem"
if (Test-Path $keyPath) {
  Write-Verbose "Using key pair $keyName found at path: $keyPath"
} else {
  Write-Verbose "Generate key pair $keyName and saving to: $keyPath"
  aws ec2 create-key-pair --key-name $keyName --query 'KeyMaterial' --output text | Out-File $keyPath
  if (!$?) { throw "AWS CLI returned error: $LastExitCode" }
}

Write-Verbose "Deploying cloud formation to create server stack $stackName"

$instanceType = 't3.micro'
$amiImageId = '/aws/service/ami-amazon-linux-latest/al2023-ami-kernel-6.1-x86_64'

aws cloudformation deploy `
    --template-file $stackTemplateFile `
    --stack-name $stackName `
    --tags `
      "BusinessUnit=IoT" `
      "DataClassification=Non-business" `
      "Environment=$Environment" `
    --parameter-overrides "InstanceType=$instanceType" `
      "KeyName=$keyName" `
      "AmiImageId=$amiImageId" `
      "VPCId=$($vpc.VpcId)" `
      "Ipv6SubnetId=$($ipv6Subnet.SubnetId)" `
      "DualStackSubnetId=$($dualStackSubnet.SubnetId)" `
      "DualStackIpv6Address=$dsIpv6Address" `
      "DualStackIpv4Address=$dsIpv4Address"
if (!$?) { throw "AWS CLI returned error: $LastExitCode" }

# aws cloudformation delete-stack --stack-name 'leshan-dev'

# Retrieve the output values
$stackOutputs = aws cloudformation describe-stacks `
    --stack-name $networkStackName `
    | ConvertFrom-Json `
    | Select-Object -ExpandProperty Stacks

Write-Host "Stack outputs:`n$($stackOutputs | ConvertTo-Json -Depth 10)"

Write-Host "To connect: ssh -i $keyPath <host-dns-name>"
