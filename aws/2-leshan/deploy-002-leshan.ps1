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
    [string]$Environment = $ENV:DEPLOY_ENVIRONMENT ?? 'Dev'
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
$publicSubnetName = "$networkStackName-public-subnet-01".ToLowerInvariant()

# Check if profile and region environment variables are set
if (-not $ENV:AWS_PROFILE) {
  Write-Host 'You need to configure the default profile, e.g. $ENV:AWS_PROFILE="AWSAdministratorAccess-632781653573"'
  throw 'You need to configure the default profile, e.g. $ENV:AWS_PROFILE="AWSAdministratorAccess-632781653573"'
}
if (-not $ENV:AWS_DEFAULT_REGION) {
  Write-Host 'You need to configure the default region, e.g. $ENV:AWS_DEFAULT_REGION="ap-southeast-2"'
  throw 'You need to configure the default region, e.g. $ENV:AWS_DEFAULT_REGION="ap-southeast-2"'
}
  
# Deploy the network stack

Write-Verbose "Getting VPC $vpcName"
$vpc = aws ec2 describe-vpcs `
    --filters "Name=tag:Name,Values=$vpcName" `
    | ConvertFrom-Json `
    | Select-Object -ExpandProperty Vpcs `
    | Select-Object -First 1

Write-Verbose "Getting public subnet $publicSubnetName" 
$subnet = aws ec2 describe-subnets `
  --filters "Name=tag:Name,Values=$publicSubnetName" "Name=vpc-id,Values=$($vpc.VpcId)" `
  | ConvertFrom-Json `
  | Select-Object -ExpandProperty Subnets `
  | Select-Object -First 1   


Write-Verbose

aws ec2 create-key-pair --key-name MyKeyPair --query 'KeyMaterial' --output text > MyKeyPair.pem

Write-Verbose "Deploying cloud formation to create server stack $stackName"

$instanceType = 't3.micro'
$imageId = 'resolve:ssm:/aws/service/ami-amazon-linux-latest/amzn2-ami-hvm-x86_64-gp2'

aws cloudformation deploy `
    --template-file $stackTemplateFile `
    --stack-name $stackName `
    --tags `
      "BusinessUnit=IoT" `
      "DataClassification=Non-business" `
      "Environment=$Environment" `
    --parameter-overrides "InstanceType=$instanceType" KeyName=my-key "VPCId=$($vpc.VpcId)" "SubnetId=$($subnet.SubnetId)" "ImageId=$imageId"

# Retrieve the output values
$stackOutputs = aws cloudformation describe-stacks `
    --stack-name $networkStackName `
    | ConvertFrom-Json `
    | Select-Object -ExpandProperty Stacks

Write-Host "Stack outputs:`n$($stackOutputs | ConvertTo-Json -Depth 10)"
