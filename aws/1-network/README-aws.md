AWS network infrastructure
=================================

Contents
--------

Creates a virtual private cloud (VPC) with several different subnets.

The core network uses an Amazon provided IPv6 range, and a 10.0.0.0/16 private range for the dual stack subnets.

Public subnets have a routing table with:
* Default IPv6 route via Internet Gateway, attached to the VPC
* NAT64 route via NAT Gateway, which has a IPv4 address in the dual stack public subnet 2
* For IPv4 resources, via Internet Gateway, which will assign an IPv4 address

Private subnets with outbound access have a routing table with:
* Default IPv6 route via Egress Only Internet Gateway
* NAT64 route via NAT Gateway
* For IPv4 only resources, NAT44 via NAT Gateway

Subnets:

* Public Subnet 1 (IPv6 only) - suffix 00
  - gets the first /64 subnet, which ends with 00
  - uses DNS64 for IPv4 destinations
* Public Subnet 2 (dual stack) - suffix 01 
  - gets the second /64 subnet, which ends with 01
  - IPv4 private range uses 10.0.1.0/24 for internal addresses; internet gateway assigns public addresses as needed
  - does not have DNS64, so dual stack clients will use their IPv4 address for connections
* Private Subnet 1 (IPv6 only) - suffix 02
  - gets the third /64 subnet, which ends with 02
  - outbound only access
  - has DNS64 for IPv4 destnations
* Private Subnet 2 (dual stack) - suffix 03
  - gets the fourth /64 subnet, which ends with 03
  - IPv4 private range uses 10.0.3.0/24
  - outbound only access: IPv6 is firewall based, IPv4 is NAT based
  - configured with DNS64, so dual stack machines use NAT64 instead of NAT44
* Private Subnet 3 (IPv6 only) - suffix 04
  - gets the fifth /64 subnet, which ends with 04
* Private Subnet 4 (dual stack) - suffix 05
  - gets the sixth /64 subnet, which ends with 05
  - IPv4 private range uses 10.0.5.0/24

Subnets 3 & 4 are internal only, with no outbound access.


Requirements
------------

* AWS account
* AWS CLI, for deployment tools (`winget install Amazon.AWSCLI`)
* PowerShell SecretStore & SecretManagement, for storing secrets

Running scripts
---------------

Set up parameters

```powershell
aws configure sso
```

Use:
* Session: sso-datahub (or make up your own name)
* Start URL: https://d-9a673a3182.awsapps.com/start#
* Region: us-east-2
* Scopes: (default) sso:account:access
* Select the account ??? (???) with role AWSAdministratorAccess
* Default region, format, and profile name (<your profile>)

Configure the default settings:

```powershell
$ENV:AWS_PROFILE="<your profile>"
$ENV:AWS_DEFAULT_REGION="ap-southeast-2"
$ENV:DEPLOY_ENVIRONMENT = 'Dev'
$VerbosePreference = 'Continue'
```

Log in, if needed (e.g. configuration was previously set up)

```powershell
aws sso login
```

Then run all the scripts:

```powershell
./deploy-001-vpc-network.ps1
```
