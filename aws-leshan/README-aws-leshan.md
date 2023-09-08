Leshan server on AWS using Cloud Development Kit (CDK)
======================================================

The server is running dual-stack, as AWS only provides IPv4 DNS names (for HTTPS). Also, public networks need to have IPv4 available for NAT64 gateways.

The server uses Caddy to automatically provision a TLS certificate for the server host address, with HTTPS traffic forwarded to the Leshan server web UI. Port 80 is also open, so that Caddy can use Let's Encrypt to automatically get a certificate.

Caddy also applies basicauth to the website, requiring a username and password. This is done over HTTPS, so the password is secure.

Requirements:
* PowerShell 7+, for running scripts (`winget install Microsoft.PowerShell`)
* Git, for source code (`winget install Git.Git --source winget`)
* Visual Studio Code, or similar editor (`winget install Microsoft.VisualStudioCode`)
* AWS CLI, for deployment tools (`winget install Amazon.AWSCLI`)
* Node (`winget install OpenJS.NodeJS`), used to install Cloud Development Kit
* AWS account

### Prerequisites

* Configure AWS single sign on
* Install the Cloud Developmen Kit (`npm install -g aws-cdk`) and ensure it is bootstrapped for your account.

### Sign in to AWS

To deploy, first configure your AWS SSO parameters, and sign in, e.g. (replacing the profile/region as needed):

```powershell
$ENV:AWS_PROFILE="AWSAdministratorAccess-744827226675"
$ENV:AWS_DEFAULT_REGION="ap-southeast-2"
aws sso login
```

### Preparing a key pair

To access the utility server via SSH, you need to prepare a key pair to use when creating the instance. You can check if you have an existing private key file: `ls ~/.ssh/`.

You can check the key pairs that have been created in AWS via: `(aws ec2 describe-key-pairs | ConvertFrom-Json).KeyPairs | Format-Table KeyName, CreateTime`

If you don't, then you need to create a named key pair in AWS, and store the private key:

```powershell
$keyName = "leshan-demo-key".ToLowerInvariant()
$sshFolder = "~/.ssh"
$keyPath ="$sshFolder/$keyName.pem"
aws ec2 create-key-pair --key-name $keyName --query 'KeyMaterial' --output text | Out-File $keyPath
```

### Deploy the CloudFormation stacks

Parameters also need to be configured for CDK, then you can deploy the network stack:

```powershell
$ENV:CDK_DEFAULT_ACCOUNT="744827226675"
$ENV:CDK_DEFAULT_REGION="ap-southeast-2"
cd aws-lwm2m-demo
cdk deploy Lwm2mDemoNetworkStack
```

Then to deploy the server stack you need to supply the web UI password that will be used:

```powershell
cdk deploy Lwm2mDemoServerStack --parameters Lwm2mDemoServerStack:basicPassword=P@ssword1
```

### Get the server details

You can query the instance details via the AWS CLI:

```powershell
$leshanStack = aws cloudformation describe-stacks --stack-name Lwm2mDemoServerStack | ConvertFrom-Json
$leshanInstance = aws ec2 describe-instances --instance-ids $leshanStack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
$leshanInstance.Reservations.Instances.NetworkInterfaces.Ipv6Addresses.Ipv6Address, $leshanInstance.Reservations.Instances.PublicDnsName
```

AWS will provide an IPv6 address for the server (with the suffix we configured of `:100d`), but does not generate a DNS entry for it. To access the admin console via HTTPS (automatically provided by Lets Encrypt) you need to use the IPv4-based DNS name.


### Stop and start

There are script to stop (to save money) and restart (e.g. each day after the automatic shutdown) the server.

```powershell
$leshanStack = aws cloudformation describe-stacks --stack-name Lwm2mDemoServerStack | ConvertFrom-Json
aws ec2 stop-instances --instance-ids $leshanStack.Stacks[0].Outputs[0].OutputValue

aws ec2 start-instances --instance-ids $leshanStack.Stacks[0].Outputs[0].OutputValue
```

After restarting you need to log on and run the Leshan server (`java -jar ~/leshan-server/leshan-server-demo.jar`)

### Cleanup

When you are finished, you can destroy the CloudFormation stack deployments.

```powershell
cdk destroy Lwm2mDemoNetworkStack Lwm2mDemoServerStack
```

Testing the Leshan Server
-------------------------

### Connecting to the server via SSH

You can use SSH, with the private key, to access the server directly:

```powershell
$leshanStack = aws cloudformation describe-stacks --stack-name Lwm2mDemoServerStack | ConvertFrom-Json
$leshanInstance = aws ec2 describe-instances --instance-ids $leshanStack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
ssh -i ~/.ssh/leshan-demo-key.pem "ec2-user@$($leshanInstance.Reservations.Instances.NetworkInterfaces.Ipv6Addresses.Ipv6Address[1])"
```

Then if necessary run the Leshan server, from the remote shell:

```bash
nohup java -jar /home/ec2-user/leshan-server/leshan-server-demo.jar &
```

### Viewing the Leshan web UI

The web portal is accessible via HTTPS, using `$leshanInstance.Reservations[0].Instances.PublicDnsName`

You will be prompted with to enter a username ('iotadmin') and the web password.

### Download the Leshan demo client

You can use the Leshan demo client to test.

Install Java Runtime Environment if needed:

```
sudo apt install default-jre
```

Download the test client to a working folder:

```
cd ../temp
wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-client-demo.jar
```

### Configuring pre-shared key security

Generate the pre-shared key (PSK) ID and key, e.g.

```powershell
$id = "urn:imei:3504577901234567"
$key = ((Get-Random -Max 0xff -Count 32|ForEach-Object ToString X2) -join '')
$id, $key
```

In the Leshan Web UI, go to Security > Add new client security configuration, and enter the following:

* Client endpoint: urn:imei:3504577901234567 (as above)
* Security mode: Pre-Shared Key
* Identity: urn:imei:3504577901234567
* Key: (as generated)

Click Create, and the endpoint will be added to the list.


### Running the client

Run the demo client, passing in the address of the Azure Leshan server.

```powershell
$leshanStack = aws cloudformation describe-stacks --stack-name Lwm2mDemoServerStack | ConvertFrom-Json
$leshanInstance = aws ec2 describe-instances --instance-ids $leshanStack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
$leshanInstance.Reservations[0].Instances.Ipv6Address
java -jar ./leshan-client-demo.jar -n $id -i $id -p $key -u "coaps://[$($leshanInstance.Reservations.Instances.NetworkInterfaces.Ipv6Addresses.Ipv6Address[1])]:5684"
```

In the web UI, you will be able to see the device connected, with the client address and security indicator. From the device screen you can operate functions like reading values from the client.


### Troubleshooting

If the server isn't working you can check the cloud init logs

```bash
cat /var/log/cloud-init-output.log
```

And also the Caddy proxy logs:

```bash
journalctl -u caddy --no-pager
```

Removing SSH known host (locally):

ssh-keygen -R $leshanInstance.Reservations.Instances.NetworkInterfaces.Ipv6Addresses.Ipv6Address[1]
