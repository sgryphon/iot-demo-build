AWS (via Cloud Development Kit)
================================

### Requirements

* PowerShell 7+, for running scripts (`winget install Microsoft.PowerShell`)
* Git, for source code (`winget install Git.Git --source winget`)
* Visual Studio Code, or similar editor (`winget install Microsoft.VisualStudioCode`)
* AWS CLI, for deployment tools (`winget install Amazon.AWSCLI`)
* Node (`winget install OpenJS.NodeJS`), used to install Cloud Development Kit
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
languages and then provisioning the infrastructure through CloudFormation. These examples use TypeScript, as the Node.js host is already installed for the CDK tools.

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


App: Landing template with IPv6
----------------------

### Deploy the network stack

Run the app with the Cloud Development Kit to deploy the network stack:

```powershell
$ENV:CDK_DEFAULT_ACCOUNT="744827226675"
$ENV:CDK_DEFAULT_REGION="ap-southeast-2"
cdk synth
cdk deploy AwsLandingStack-dev
```

See the next section for how to deploy a utility server.

### Initial creation

Init the app template:

```powershell
mkdir aws-landing
cd aws-landing
cdk init app --language typescript
```

Then add  `!bin` to the `.gitignore`.

The level 2 constructs for Virtual Private Clouds simplify configuration setting a number of defaults and automatically creating one public subnet and one private subnet with egress across multiple availability zones, and configures the needed routing and gateways including Network Address Translation (NAT).

However the default does not include IPv6 support.

#### Adding IPv6

The network configuration can be put into a separate `network-layer.ts`, with configuration settings provided by `aws-landing.ts` (this makes it easier to reuse).

The template does not cover all options available, and is coded for the public network to be dual stack, with the private (with egress) network being IPv6 only, although different combinations are available.

Ideally in the future, the library level 2 VPC will support a `vpcProtocol` property than can be set to `VpcProcotol.DUAL_STACK` to configure the defaults (with `VpcProtocol.IPV4` behaving the same as the existing, to remain backwards compatible). Note that IPv4 addresses are mandatory for the VPC, although you don't need to use them in subnets (however public subnets need to have IPv4 for NAT gateways).

Defaults for an dual stack VPC should be:
* Continue to use the passed in IPv4 addresses, and create one set of networks per availability zone (with the number of zones able to be overridden). IPv4 subnets need to be calculate to carefully balance the number of subnets vs the number of addresses in each subnet, with existing overrides for manual configuration of each availability zone set of subnets.
* Default subnets should match the VPC protocol, i.e. default to dual stack for a dual stack VPC.

Differences at the VPC level:
* Add a tag for the VPC protocol that was used.
* Assign two Amazon-provided IPv6 /56 ranges (allowing 256 subnets each). Because of the significantly larger IPv6 address space we can freely add far more addresses that IPv4 without danger of collission.
* Create an egress-only internet gateway to use with the VPC.
* Allow overrides, similar to IPv4, to use IPAM assigned ranges.

If we need more subnets for IPv6, we can just assign more ranges (unlike IPv4 where we would need to carefully manage subnet size), so encourage this by initially assigning at least two ranges (similar to how best practice uses multiple availability zones). This allows us to assign one range for public subnets, and the second for private subnets.

Differences at the Subnet level, for dual stack subnets:
* Assign a /64 subnet, based on the allocated ranges. Rather than a specific CIDR block, all subnets are /64 in size, which means all we need is the number/index of the subnet, e.g. subnet 0, 1, 2, etc. The first 256 subnets are from the first allocated /56, and then the next from the second allocated /56, etc.
* This means for the default 3 availability zones, the public subnets are index 0, 1, 2 and the private subnets are index 256, 257, 258 (which are 0, 1, 2 in the second block).
* Add a dependency on the relevant IPv6 allocation (needed to change/destroy in the correct sequence)
* Auto-assign IPv6 on creation
* Turn off map public IPv4 on launch -- Public IPv4 addresses are now charged, so we only want to add them (manually) when needed
* Enable private DNS names -- We can get the instance ID as an output from CloudFormation and use that to build the DNS name to reference the machine, without having to know the machine IP address.
* Enable DNS64 -- This allows IPv6 only machines to still have outbound NAT64 access to IPv4 only external addresses. It does mean dual stack machines will preference NAT64 (over NAT44).
  * For a flexible solution you will need  configuration property to disable this, for rare situations where DNS64 breaks an application and the app/machine can't be configure as single stack IPv4 (e.g. multi-purpose machine).

Note that public networks that contain NAT gateways need to be dual stack, so that the gateway can be assigned an IPv4 address, even if it is only used for NAT64.

Public network route configuration:
* IPv6 default `::/0` needs to go to the internet gateway.
* NAT64 `64:ff9b::/96` needs to route to a NAT gateway. If you have one gateway per public network (e.g. per availability zone), then use that; if the number of gateways is less, then they can be assigned round-robin (like for private subnets). Note that you may still want a NAT64 route, even if you don't have DNS64 enabled, as a server/device may have it's own DNS settings (e.g. us a public DNS64 server).
* IPv4 default `0.0.0.0` routed to the internet gateway (same as existing).

Private networks with egress route configuration:
* IPv6 default `::/0` needs to go to the egress only internet gateway.
* NAT64 `64:ff9b::/96` needs to route to a NAT gateway..
* IPv4 default `0.0.0.0` is routed to a NAT gateway (same as existing). Both NAT64 and NAT44 can use the same gateway, either per availabilty zone or a round robin configuration.

#### Private IPv6 only subnets

Private subnets should be IPv6 only where possible.

IPv6 subnets are much easier to allocate, as you don't need to worry about balancing the subnet mask for number of hosts vs number of subnets. Each IPv6 /64 subnet can have vastly more hosts than IPv4, and you can create as many subnets as you need (assigning as many /56 blocks as you need). You can assign more subnets than available across all IPv4 private blocks, and still only use a fraction of the address space, without any address collission.

All major operating systems, applications, and programming languages suppport IPv6, although some software may not.

For egress access to external IPv4 addresses, in most cases NAT64 (with DNS64) will be no different than using NAT44. In both cases you have to use NAT.

To support this with the level 2 VPC construct, only a few additional lines would be needed, after setting the `vpcProtocol` to `VpcProcotol.DUAL_STACK`, you would configure the default subnets per availability zone to be one of type `SubnetType.PUBLIC` with `subnetProtocol` of `SubnetProtocol.DUAL_STACK` and one of `SubnetType.PRIVATE_WITH_EGRESS` with `SubnetProtocol.IPV6`.

The default subnet mask allocation for IPv4 can be automatically split on the number of subnets, but for IPv6 you simply split the allocated block and the assign sequentially based on network type.

e.g. With the default 2 blocks and two network types, the first type would get 0, 1, 2, etc, and the second type get 256, 257, 258, etc (in the second block)

The differences in subnet configuration:
* Set `ipv6Native`
* Do not assign `cidrBlock`

Routing table is above, except the IPv4 routes are not needed.

The sample deloyment uses public dual stack and private IPv6 only subnets, letting the default level 2 VPC construct create the subnets, and allocate IPv4 ranges, and then modifying the public and private subnets accordingly.

The existing IPv4 routes for the private networks are left as is, although they won't do anything.

#### Set up multiple stack deployments

To support multiple stacks, e.g. different environments or regions, with custom IPv4 settings, modify `bin/aws-landing.ts` to deploy to multiple stacks. The IPv4 addresses need to fit into your allocation plan.

Note that different IPv6 settings are not needed, as the allocated /56 ranges from Amazon will already be unique.


Landing template - Utility server
----------------------

Additional stacks are also provided inside the CDK application to provision out administration/utility servers, to be able to example the network.

One server is deployed into the public dual stack network, and can be used as a jump host to access the second server in the private network.

### Preparing a key pair

To access the utility server via SSH, you need to prepare a key pair to use when creating the instance.

You can check if you have an existing key file: `ls ~/.ssh/`.

You can check the key pairs that have been created in AWS via: `(aws ec2 describe-key-pairs | ConvertFrom-Json).KeyPairs | Format-Table KeyName, CreateTime`

If you don't, then you need to prepare and create a key pair in AWS. After the machine is created, you can log in an modify the keys used, but if you lose the key then you won't be able to log in (and will need to recreate the machine).

```powershell
$keyName = "utility-${ENV:DEPLOY_ENVIRONMENT}-key".ToLowerInvariant()
$sshFolder = "~/.ssh"
$keyPath ="$sshFolder/$keyName.pem"
aws ec2 create-key-pair --key-name $keyName --query 'KeyMaterial' --output text | Out-File $keyPath
```

### Specifying the address suffix

The actual IPv6 address range assigned is not known until deployment time, but by using the CIDR CloudFormation function, along with array operations we can construct an IP address with a specific suffix.

Because we know the structure of the subnets created, we can specify the subnet index, where dividing by 256 gives us the block, and the remainder is the index within that block.

Subnets are always /64 in length, so we know the CIDR function will return a result like "W:X:Y:Z:0:0:0:0/64" (it returns the long form), so we can split on the ":" and use the first four components to create an address with a fixed suffix.

Althought the output CloudFormation is a bit messy (the selection functions are repeated 4 times), it can give us fixed server addresses like "W:X:Y:Z::100d"

(If you don't want a fixed address, you can just let AWS auto-assign a suffix).

### Server configuration

When creating the security group for the server, note that you need to allow both outbound IPv4 and IPv6 (separately). For testing purposes it can be useful to allow ICMP/Ping. When adding rules for other ports, e.g. if you allow HTTP/S, also make sure to configure both IPv6 and (if needed) IPv4.

Assign the fixed IPv6 address calculated as above (or you can let it auto-assign an address).

If you want to assign a public IPv4 address, then this needs to be done manually (assigning an Elastic IP), as automatic mapping is turned off (public IPv4 addresses are now charged).

### Deploying the server

Once the key is ready, and the main network has been deployed, you can deploy the utility server:

```powershell
cdk deploy UtilityServer-Public-dev
```

CloudFormation does not support output of the IPv6 address, but can output the InstanceID, which can be used to get the address via the AWS CLI:

```powershell
$publicStack = aws cloudformation describe-stacks --stack-name UtilityServer-Public-dev | ConvertFrom-Json
$publicInstance = aws ec2 describe-instances --instance-ids $publicStack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
$publicInstance.Reservations[0].Instances.Ipv6Address, $publicInstance.Reservations[0].Instances.PublicIpAddress
ping $publicInstance.Reservations[0].Instances.Ipv6Address
```

You can then use SSH, with the private key, to access the server:

```powershell
$publicUtilityIpv6 = $publicInstance.Reservations[0].Instances.Ipv6Address
ssh -i ~/.ssh/utility-dev-key.pem "ec2-user@$publicUtilityIpv6"
```

### Utility server on the private network

Once the key is ready, and the main network has been deployed, you can deploy the utility server:

```powershell
cdk deploy UtilityServer-Private-dev
```

The private server will have an IPv6 address, but it will not be directly accessible (it has egress only routing):

```powershell
$privateStack = aws cloudformation describe-stacks --stack-name UtilityServer-Private-dev | ConvertFrom-Json
$privateInstance = aws ec2 describe-instances --instance-ids $privateStack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
$privateInstance.Reservations[0].Instances.Ipv6Address
```

#### Connecting to the private server

To use the public server as a jump box, first upload the SSH key using SCP (note the different syntax from SSH), and then connect to the public jump box via SSH:

```powershell
scp -i ~/.ssh/utility-dev-key.pem "${ENV:USERPROFILE}\.ssh\utility-dev-key.pem" "ec2-user@[${publicUtilityIpv6}]:/home/ec2-user/.ssh/utility-dev-key.pem"
ssh -i ~/.ssh/utility-dev-key.pem "ec2-user@$publicUtilityIpv6"
```

From the public server, you can ping to the private server. To connect, you need to set the permissions on the key to restrict access, and then connect to the internal server:

```bash
ping 2406:da1c:c1b:2601::2001
chmod 400 ~/.ssh/utility-dev-key.pem

ssh -i ~/.ssh/utility-dev-key.pem "ec2-user@2406:da1c:c1b:2601::2001"
```

Once on the internal server, you can check the configuration, and that it has outbound connectivity to both IPv6 and IPv4:

```bash
ip addr
ping -c 3 www.google.com
ping -c 3 v4.ipv6-test.com
```
