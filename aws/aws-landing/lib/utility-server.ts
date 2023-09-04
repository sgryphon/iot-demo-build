import { Construct } from 'constructs';
import { AmazonLinuxCpuType, CfnEIP, CfnEIPAssociation, IVpc, Instance, InstanceType, MachineImage, Peer, Port, SecurityGroup, SubnetSelection, UserData } from 'aws-cdk-lib/aws-ec2';
import { ManagedPolicy, PolicyDocument, PolicyStatement, Role, ServicePrincipal } from 'aws-cdk-lib/aws-iam';

export class UtilityServer extends Construct {

  public instance: Instance;
  public securityGroup: SecurityGroup;

  constructor(scope: Construct, id: string, props?: UtilityServerProps) {
    super(scope, id);

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
    this.securityGroup = new SecurityGroup(this, 'UtilityServerSecurityGroup', {
      vpc: props!.vpc!,
      description: 'Security Group for utility server',
      allowAllOutbound: true,
      allowAllIpv6Outbound: true,
    });

    // Utility server allows SSH, ICMP/Ping, HTTP, and HTTPS inbound
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(22), 'Allow IPv6 SSH (22) inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(80), 'Allow IPv6 HTTP (80) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.tcp(443), 'Allow IPv6 HTTPS (443) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv6(), Port.allIcmpV6(), 'Allow IPv6 ICMP inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(22), 'Allow IPv4 SSH (22) inbound');
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(80), 'Allow IPv4 HTTP (80) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.tcp(443), 'Allow IPv4 HTTPS (443) inbound')
    this.securityGroup.addIngressRule(Peer.anyIpv4(), Port.icmpPing(), 'Allow IPv4 ICMP ping inbound');

    // Create the instance
    // See: https://github.com/aws-samples/aws-cdk-examples/blob/master/typescript/ec2-instance/src/server.ts

    // Default
    const userData = UserData.forLinux();

    this.instance = new Instance(this, 'Instance', {
      instanceType: props?.instanceType!,
      keyName: props?.keyName,
      machineImage: MachineImage.latestAmazonLinux2023({
        cachedInContext: false,
        cpuType: AmazonLinuxCpuType.X86_64,
      }),
      role: serverRole,
      securityGroup: this.securityGroup,
      userData: userData,
      vpc: props!.vpc!,
      vpcSubnets: props!.subnets!,
    });

    this.instance.instance.ipv6Addresses = [{
      ipv6Address: props?.ipv6Address!
    }];

    if (props?.mapPublicIpv4) {
    // Assign Elastic IPv4
      let eip = new CfnEIP(this, "Ip");
      let ec2Assoc = new CfnEIPAssociation(this, "Ec2Association", {
        eip: eip.ref,
        instanceId: this.instance.instanceId
      });
    }
  }
}

export interface UtilityServerProps {
  readonly instanceType: InstanceType;
  readonly ipv6Address: string;
  readonly keyName: string;
  readonly mapPublicIpv4: boolean;
  readonly subnets?: SubnetSelection;
  readonly vpc?: IVpc;
}
