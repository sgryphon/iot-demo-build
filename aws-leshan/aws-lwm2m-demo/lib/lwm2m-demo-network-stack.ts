import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { CfnSubnet, 
  CfnVPC, 
  CfnVPCCidrBlock,
  IIpAddresses,
  NatProvider,
  RouterType,
  Subnet,
  SubnetType,
  Vpc } from 'aws-cdk-lib/aws-ec2';
import { CfnOutput, Fn, Tags } from 'aws-cdk-lib';

export class Lwm2mDemoNetworkStack extends cdk.Stack {

  public readonly vpc: Vpc;

  constructor(scope: Construct, id: string, props?: Lwm2mDemoNetworkStackProps) {
    super(scope, id, props);

    // The default `Vpc` does not support IPv6, so add changes to make it dual stack,
    // with dual stack public networks (need an IPv4 address for NAT).

    // This is the same as the default NAT provider, but we want a reference, so create here
    // This will create one public network, and one private network.
    // We will only use the public network, but are keeping the private one so that a NAT gateway is created
    const maxAzs = 1;
    const natProvider = NatProvider.gateway();
    this.vpc = new Vpc(this, 'VPC', {
        ipAddresses: props?.ipv4PrivateAddresses,
        maxAzs: maxAzs,
        natGatewayProvider: natProvider,
    });
    Tags.of(this.vpc).add('aws-cdk-ex:vpc-protocol', 'DualStack', { includeResourceTypes: [CfnVPC.CFN_RESOURCE_TYPE_NAME] });

    // Assign an initial IPv6 /56 blocks (unlike IPv4 we can freely add as many as we need)
    // Note: A virtual private cloud (VPC) must be dual stack, i.e. must have IPv4 addresses (assigned above), even if not used in any networks
    const ipv6PublicBlock = new CfnVPCCidrBlock(this.vpc, 'Ipv6PublicBlock', {
        amazonProvidedIpv6CidrBlock: true,
        vpcId: this.vpc.vpcId
    });

    // Update public subnets to dual stack, to support IPv6
    // Networks are dual stack, as they need IPv4 for NAT
    const ipv6CidrBlocks = Fn.cidr(
      Fn.select(0, this.vpc.vpcIpv6CidrBlocks),
      maxAzs,"64");
    this.vpc.publicSubnets.forEach((subnet, index) => {
        Tags.of(subnet).add('aws-cdk-ex:subnet-protocol', 'DualStack', { includeResourceTypes: [CfnSubnet.CFN_RESOURCE_TYPE_NAME] });

        const cfnSubnet = subnet.node.defaultChild as CfnSubnet;
        cfnSubnet.assignIpv6AddressOnCreation = true;
        // Enabling DNS64 allows IPv6 only clients (on the dual stack network) to access external IPv4.
        // This also preferences IPv6, e.g. for routing, even from dual stack machines.
        // TODO: Need a configuration property to disable this, for rare situations where DNS64 breaks an application 
        // and the app/machine can't be configure as single stack IPv4 (e.g. multi-purpose machine)
        cfnSubnet.enableDns64 = true;
        // Use the first IPv6 block for the public networks
        // We don't need to worry about sizes, as a /64 subnet is already huge, and we can add as many /56 blocks as we need
        // (Unlike IPv4 where we need to carefully balance the number of subnets vs addresses within each subnet, but shifting the size)
        cfnSubnet.ipv6CidrBlock = Fn.select(index, ipv6CidrBlocks);
        // Public IPv4 will start being charged, so don't automatically assign (as we have IPv6 assigned)
        cfnSubnet.mapPublicIpOnLaunch = false;
        cfnSubnet.privateDnsNameOptionsOnLaunch = {
            EnableResourceNameDnsAAAARecord: true,
            EnableResourceNameDnsARecord : true
        };
        cfnSubnet.addDependency(ipv6PublicBlock);

        // We know these are created subnets (not other types), so can cast
        // Add default route for IPv6 to the internet gateway
        const sn = subnet as Subnet;
        sn.addRoute('Ipv6Default', {
            destinationIpv6CidrBlock: '::/0',
            routerId: this.vpc.internetGatewayId!,
            routerType: RouterType.GATEWAY,
        });

        // Add route for NAT64
        // NOTE: Our VPC above is configured one gateway per public network
        // TODO: Handle where number of NAT < maxAzs
        // const az = subnet.availabilityZone;
        const natGatewayId = natProvider.configuredGateways[index].gatewayId;
        sn.addRoute('Nat64', {
            destinationIpv6CidrBlock: '64:ff9b::/96',
            routerId: natGatewayId,
            routerType: RouterType.NAT_GATEWAY,
        });
    });

    // NOTE: We are not modifying the private network, as we will not be using it,
    // but could change it similarly to be dual stack (or single stack IPv6)

    Tags.of(this).add('Owner', 'IoT Demo');
    Tags.of(this).add('Classification', 'Confidential');

    new CfnOutput(this, 'VpcId', { value: this.vpc.vpcId });
    new CfnOutput(this, 'PublicSubnetIds', { value: this.vpc.publicSubnets.map(x => x.subnetId).join(',') });
  }
}

export interface Lwm2mDemoNetworkStackProps extends cdk.StackProps {
  readonly ipv4PrivateAddresses?: IIpAddresses;
}
