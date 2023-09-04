AWS (via Cloud Development Kit)
================================

### Requirements

* PowerShell 7+, for running scripts (`winget install Microsoft.PowerShell`)
* Git, for source code (`winget install Git.Git --source winget`)
* Visual Studio Code, or similar editor (`winget install Microsoft.VisualStudioCode`)
* AWS CLI, for deployment tools (`winget install Amazon.AWSCLI`)
* Node (`winget install OpenJS.NodeJS`, or another language to use with AWS CDK)
* AWS account


Amazon Web Services (AWS)
-------------------------

Cloud infrastructure provider.

These folders contains stacks for the following components:

* Core networking infrastructure landing zone
* Virtual machine running a Leshan LwM2M test server

### AWS developer account

You can request a AWS Training Account via Telstra Purple Technology Operations, https://it.purple.tech/hc/en-au/articles/360040960893-AWS

e.g. I have a subscription named "IoT Playground - Sly", #744827226675

The IoT Lab does not (currently) have a shared AWS account (although the Cumulocity DataHub evaluation project did).

### Log in to AWS

To access your AWS developet subscription, open the Telstra Purple AWS app:
* https://telstrapurple.awsapps.com/start#/

Open up the Command line access (you can also open the Management console to see the results).

If this is the first time accessing your subscription, you will want to configure single sign on:

```powershell
aws configure sso
```

Use:
* Session: sso-datahub (or make up your own name)
* Start URL: https://d-9a673a3182.awsapps.com/start#
* Region: us-east-2
* Scopes: (default) sso:account:access
* Select the account, e.g. IoT Playground - Sly (7448-2722-6675) with role AWSAdministratorAccess
* Default region, format, and profile name (e.g. AWSAdministratorAccess-632781653573)

This will create a profile entry in `~\.aws\config`.

Configure the default settings:

```powershell
cd aws
$ENV:AWS_PROFILE="AWSAdministratorAccess-744827226675"
$ENV:AWS_DEFAULT_REGION="ap-southeast-2"
$ENV:DEPLOY_ENVIRONMENT = 'Dev'
$VerbosePreference = 'Continue'
```

Log in, if needed, e.g. configuration was previously set up (this will us the default profile set above):

```powershell
aws sso login
```

Cloud Development Kit
--------------

The Cloud Development Kit (CDK) is a framework for defining Infrastructure-as-Code using programming
languages, e.g. TypeScript, and then provisioning it through CloudFormation.

For more details see: https://docs.aws.amazon.com/cdk/

### Install

The AWS CDK tools can be installed globally via node:

```powershell
npm install -g aws-cdk
cdk --version
```

### Bootstrap

The AWS CDK uses a dedicated S3 bucket to manage templates. This is created automatically
in your account via bootstrapping, e.g. assuming your profile name has the account ID at the end

```powershell
$awsId = aws sts get-caller-identity | ConvertFrom-Json
cdk bootstrap "aws://$($awsId.Account)/${ENV:AWS_DEFAULT_REGION}"
```

### Make sure to include the app in source control

If necessary `cdk init` (see below) will create a git repository, however if adding to an
existing git repository (such as this one), make sure that all files are tracked.

In particular, CDK uses a folder `bin` to store the main app file, however this folder is
commonly excluded by main `.gitignore` templates.

**Fix:** The fix is to add `!bin` to the `.gitignore` created in the app folder by `cdk init`,
so that the files are included.


App: Sample
-----------

Init an empty app:

```powershell
mkdir hello-cdk
cd hello-cdk
cdk init app --language typescript
```

Following the example to see modifications and removal: https://docs.aws.amazon.com/cdk/v2/guide/hello_world.html


App: Landing
------------

### Initial creation

Init the app template:

```powershell
mkdir aws-landing
cd aws-landing
cdk init app --language typescript
```

### Running the developed app

```powershell
$ENV:CDK_DEFAULT_ACCOUNT="744827226675"
$ENV:CDK_DEFAULT_REGION="ap-southeast-2"
cdk synth
cdk deploy AwsLandingStack-dev
```

