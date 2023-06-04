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

Then run all the scripts:

```powershell
./deploy-002-leshan.ps1
```

Accessing
---------

```powershell
$instance = (aws ec2 describe-instances --filters Name=key-name,Values=leshan-dev-key Name=instance-state-name,Values=running | ConvertFrom-Json).Reservations | Select -First 1
$instance.Instances.Ipv6Address
$instance.Instances.PublicIpAddress
``` 

You can access the machine via IPv6 SSH, to the IP address:

```powershell
ssh -i ~/.ssh/leshan-dev-key.pem ec2-user@2406:da1c:ab1:2901:93fc:9eed:d9f7:bc6e
```

Or via IPv4 to the auto-generated DNS name (based on the IPv4 address):

```powershell
ssh -i ~/.ssh/leshan-dev-key.pem ec2-user@ec2-13-236-85-52.ap-southeast-2.compute.amazonaws.com
```

You can also copy the keys to another machine to use. If you copy to a Linux machine, make sure
that you restrict the permissions, otherwise SSH won't use the key.

```powershell
cp ~/Documents/VMWareShared/iot-demo/leshan-dev-key.pem ~/.ssh/leshan-dev-key.pem
chmod 400 ~/.ssh/leshan-dev-key.pem
```


