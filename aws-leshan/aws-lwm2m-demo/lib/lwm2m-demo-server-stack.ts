import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { AmazonLinuxCpuType, 
  CfnEIP, 
  CfnEIPAssociation, 
  CloudFormationInit, 
  InitCommand, 
  InitConfig, 
  InitFile,
  InitPackage,
  InitUser,
  Instance, 
  InstanceType, 
  MachineImage, 
  Peer, 
  Port, 
  SecurityGroup, 
  SubnetSelection, 
  SubnetType, 
  Vpc } from 'aws-cdk-lib/aws-ec2';
import { CfnOutput, CfnParameter, Duration, Fn, Stack } from 'aws-cdk-lib';
import { ManagedPolicy, PolicyDocument, PolicyStatement, Role, ServicePrincipal } from 'aws-cdk-lib/aws-iam';

export class Lwm2mDemoServerStack extends cdk.Stack {

  public readonly instance: Instance;
  public readonly securityGroup: SecurityGroup;

  constructor(scope: Construct, id: string, props?: Lwm2mDemoServerStackProps) {
    super(scope, id, props);

    // Get password from parameter
    const basicPassword = new CfnParameter(this, "basicPassword", {
      type: "String",
      description: "Password used to secure the Leshan web interface",
    });

    // Create a role for the EC2 instance to assume.  This role will allow the instance to put log events to CloudWatch Logs
    const serverRole = new Role(this, 'serverEc2Role', {
      assumedBy: new ServicePrincipal('ec2.amazonaws.com'),
      inlinePolicies: {
        ['RetentionPolicy']: new PolicyDocument({
          statements: [
            new PolicyStatement({
              resources: ['*'],
              actions: ['logs:PutRetentionPolicy'],
            }),
          ],
        }),
      },
      managedPolicies: [
        ManagedPolicy.fromAwsManagedPolicyName('AmazonSSMManagedInstanceCore'),
        ManagedPolicy.fromAwsManagedPolicyName('CloudWatchAgentServerPolicy'),
      ],
    });

    // Create a security group for the server
    this.securityGroup = new SecurityGroup(this, 'Lwm2mDemoSecurityGroup', {
      vpc: props!.vpc!,
      description: 'Security Group for LwM2M demo server',
      allowAllOutbound: true,
      allowAllIpv6Outbound: true,
    });
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(22), 'Allow IPv6 SSH (22) inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(80), 'Allow IPv6 HTTP (80) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(443), 'Allow IPv6 HTTPS (443) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.udpRange(5683, 5684), 'Allow IPv6 LwM2M (5683-5684) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.allIcmpV6(), 'Allow IPv6 ICMP inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(22), 'Allow IPv4 SSH (22) inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(80), 'Allow IPv4 HTTP (80) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(443), 'Allow IPv4 HTTPS (443) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.udpRange(5683, 5684), 'Allow IPv4 LwM2M (5683-5684) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.icmpPing(), 'Allow IPv4 ICMP ping inbound');

    // Assign an IPv4 address
    var eip = new CfnEIP(this, "Ip");
    // Derive host name from IPv4
    var hostName = props?.hostName;
    if (!hostName) {
      hostName = 'ec2-' + Fn.join('-', Fn.split('.', eip?.attrPublicIp!)) + '.'
        + Stack.of(this).region + '.compute.amazonaws.com'
    }
    const init = CloudFormationInit.fromConfigSets({
      configSets: { default: ['caddy', 'leshan'], },
      configs: {
        // Caddy, see: https://caddyserver.com/docs/running
        caddy: new InitConfig([
          InitUser.fromName('caddy', { 
            homeDir: '/var/lib/caddy',
          }),
          InitFile.fromString('/etc/caddy/Caddyfile',
            hostName + ' {\n'
            + '  basicauth {\n'
            + '    iotadmin __hashed_password_base64__\n'
            + '  }\n'
            + '  reverse_proxy localhost:8080\n'
            + '}\n',
            { group: 'caddy', owner: 'caddy', }
          ),
          InitCommand.shellCommand('sudo yum -y copr enable @caddy/caddy epel-7-$(arch)'),
          InitCommand.shellCommand('sudo yum -y install caddy'),
          InitCommand.shellCommand('HASHED_PASSWORD=$(caddy hash-password --plaintext \'' + basicPassword.valueAsString + '\');'
            + ' echo $HASHED_PASSWORD;' 
            + ' sudo sed -i s:__hashed_password_base64__:${HASHED_PASSWORD//:/\\:}:g /etc/caddy/Caddyfile'),
          InitCommand.shellCommand('sudo systemctl enable --now caddy'),
        ]),
        leshan: new InitConfig([
          InitPackage.yum('java-17-amazon-corretto'),
          InitCommand.shellCommand('mkdir /home/ec2-user/leshan-server'),
          InitCommand.shellCommand('wget -O /home/ec2-user/leshan-server/leshan-server-demo.jar https://ci.eclipse.org/leshan/job/leshan-1.x/lastSuccessfulBuild/artifact/leshan-server-demo.jar'),
          //InitCommand.shellCommand('nohup java -jar /home/ec2-user/leshan-server/leshan-server-demo.jar &')
        ]),
      }
    });

    // Build IPv6 address
    const vpcBlock = 0;
    const subnetBlock = 0;
    const addressBlock = Fn.select(subnetBlock, 
      Fn.cidr(Fn.select(vpcBlock, props?.vpc?.vpcIpv6CidrBlocks!), subnetBlock + 1, "64"));
    // Results from the CIDR function have the format "2406:da1c:dc0:300:0:0:0:0/64",
    // so split on ":" and use the first four.
    const split = Fn.split(":", addressBlock)
    const ipv6Address = Fn.join(":",
      [ Fn.select(0, split), Fn.select(1, split), Fn.select(2, split), Fn.select(3, split),
        "", props?.addressSuffix! ]);

    // Other parameters
    const az = cdk.Stack.of(this).availabilityZones[0];
    const subnetSelection: SubnetSelection = {
      subnetType: SubnetType.PUBLIC,
      availabilityZones: [ az ],
    };

    const machineImage = MachineImage.latestAmazonLinux2023({
      cachedInContext: false,
      cpuType: AmazonLinuxCpuType.X86_64,
    });

    // Create the instance
    this.instance = new Instance(this, 'Instance', {
      init: init,
      initOptions: {
        ignoreFailures: true,
        timeout: Duration.minutes(10),
      },
      instanceType: props?.instanceType!,
      keyName: props?.keyName,
      machineImage: machineImage,
      securityGroup: this.securityGroup,
      role: serverRole,
      userDataCausesReplacement: true,
      vpc: props!.vpc!,
      vpcSubnets: subnetSelection,
    });
    // Assign Elastic IPv4
    const ec2Assoc = new CfnEIPAssociation(this, "Ec2Association", {
      eip: eip!.ref,
      instanceId: this.instance.instanceId
    });
    // Assign IPv6
    this.instance.instance.ipv6Addresses = [{
      ipv6Address: ipv6Address
    }];

    new CfnOutput(this, 'instanceId', { value: this.instance.instanceId });
    /*
    $stack = aws cloudformation describe-stacks --stack-name Lwm2mLeshanStack-dev | ConvertFrom-Json
    $instance = aws ec2 describe-instances --instance-ids $stack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
    $instance.Reservations[0].Instances.Ipv6Address, $instance.Reservations[0].Instances.PublicIpAddress
    */
  }
}

export interface Lwm2mDemoServerStackProps extends cdk.StackProps {
  readonly addressSuffix?: string;
  readonly hostName?: string;
  readonly instanceType?: InstanceType;
  readonly keyName?: string;
  readonly vpc?: Vpc;
}
