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
