AWS Leshan LwM2M server
=======================

Contents
--------

Creates an Elastic Compute (EC) machine, running a Leshan LwM2M server.

Requirements
------------

* AWS account
* AWS CLI, for deployment tools (`winget install Amazon.AWSCLI`)
* PowerShell SecretStore & SecretManagement, for storing secrets

Dependencies:
* Uses the networks created in `1-core`

Bring your own DNS
------------------

AWS will automatically generate a DNS named for public IPv4 addresses (based on the IP address),
but not for IPv6.

To test IPv6 with security, e.g. certificates, you need to provide your own DNS names, assign the IP addresses, and use them when creating the machine.

Once the Virtual Private Cloud network is created, an IPv6 range is allocated to it. You can assign addresses from this range as needed, so we pick an address to use from the dual stack public network. Because you get allocated an entire `/56` range per VPC we can easily assign as many addresses as we need.

In contrast, IPv4 addresses are scarce, and we need to pre-allocate a specific Elastic IP (EIP) address, to know the value. Usage of IPv4 addresses is limited to five (5) EIPs per region.

We can use the first script to create the Elastic IP, and then output the picked IPv6 address along with the EIP.

```powershell
.\deploy-001-elastic-ip.ps1
```

This will end with output like the following:

```
Assign IPv4 DNS (based on EIP eipalloc-0b5126219f9e0e830)
IPv4 address: 13.211.73.98

Assign IPv6 DNS (based on subnet core-network-dev-public-subnet-02)
IPv6 subnet: 2406:da1c:ab1:2901::/64
IPv6 address: 2406:da1c:ab1:2901::100d
Internal private IPv4 address: 10.0.1.13
```

You can use the output IP addresses to configure host names in your DNS, e.g.:

```
lwm2mdev01.demo AAAA 2406:da1c:ab1:2901::100d
lwm2mdev01v4.demo A 13.211.73.98
```

Running scripts
---------------

See `1-core` for setting up a profile with SSO parameters.

Configure the AWS CLI settings:

```powershell
$ENV:AWS_PROFILE="AWSAdministratorAccess-744827226675"
$ENV:AWS_DEFAULT_REGION="ap-southeast-2"
$ENV:DEPLOY_ENVIRONMENT = 'Dev'
$VerbosePreference = 'Continue'
```

Log in, if needed (e.g. configuration was previously set up)

```powershell
aws sso login
```

Set the configuration values to the DNS names you created (replace the same ones below),
and run the script to create the server:

```powershell
$ENV:DEPLOY_HOST_IPV6 = 'lwm2mdev01.demo.gamertheory.net'
$ENV:DEPLOY_HOST_IPV4 = 'lwm2mdev01v4.demo.gamertheory.net'
./deploy-002-leshan.ps1
```

Accessing
---------

You can access the machine via IPv6 SSH:

```powershell
ssh -i ~/.ssh/leshan-dev-key.pem ec2-user@lwm2mdev01.demo.gamertheory.net
```

You can access the machine via IPv6 SSH, to the IP address:

```powershell
ssh -i ~/.ssh/leshan-dev-key.pem ec2-user@2406:da1c:ab1:2901::100d
```

Or via IPv4 to the auto-generated DNS name (based on the IPv4 address):

```powershell
ssh -i ~/.ssh/leshan-dev-key.pem ec2-user@ec2-13-211-73-98.ap-southeast-2.compute.amazonaws.com
```

You can also copy the keys to another machine to use. If you copy to a Linux machine, make sure
that you restrict the permissions, otherwise SSH won't use the key.

```powershell
cp ~/Documents/VMWareShared/iot-demo/leshan-dev-key.pem ~/.ssh/leshan-dev-key.pem
chmod 400 ~/.ssh/leshan-dev-key.pem
```


