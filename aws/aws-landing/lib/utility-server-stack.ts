import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { CfnInstance, InstanceClass, InstanceSize, InstanceType, SubnetSelection, SubnetType, Vpc } from 'aws-cdk-lib/aws-ec2';
import { CfnOutput, Fn, Tags, Token } from 'aws-cdk-lib';
import { UtilityServer } from './utility-server';

export class UtilityServerStack extends cdk.Stack {

  public server: UtilityServer;

  constructor(scope: Construct, id: string, props?: UtilityServerStackProps) {
    super(scope, id, props);

    const az = cdk.Stack.of(this).availabilityZones[props?.availabilityZoneIndex!];
    const subnetSelection: SubnetSelection = {
      subnetType: props?.subnetType,
      availabilityZones: [ az ],
    };
    const keyName = `utility-${props?.environment}-key`.toLowerCase();

    const vpcBlock = props?.addressSubnetIndex! / 256;
    const subnetBlock = props?.addressSubnetIndex! % 256;
    const addressBlock = Fn.select(subnetBlock, 
      Fn.cidr(Fn.select(vpcBlock, props?.vpc?.vpcIpv6CidrBlocks!), subnetBlock + 1, "64"));
    // Results from the CIDR function have the format "2406:da1c:dc0:300:0:0:0:0/64",
    // so split on ":" and use the first four.
    const split = Fn.split(":", addressBlock)
    const ipv6Address = Fn.join(":",
      [ Fn.select(0, split), Fn.select(1, split), Fn.select(2, split), Fn.select(3, split),
        "", props?.addressSuffix! ]);

    this.server = new UtilityServer(this, 'Server', {
      instanceType:  InstanceType.of(InstanceClass.T3, InstanceSize.MICRO),
      ipv6Address: ipv6Address,
      keyName: keyName,
      mapPublicIpv4: props?.subnetType === SubnetType.PUBLIC,
      subnets: subnetSelection,
      vpc: props?.vpc,
    });

    Tags.of(this).add('Owner', 'IoT Lab');
    Tags.of(this).add('Classification', 'Confidential');

    new CfnOutput(this, 'instanceId', { value: this.server.instance.instanceId });
    /*
    $stack = aws cloudformation describe-stacks --stack-name UtilityServer-Public-dev | ConvertFrom-Json
    $instance = aws ec2 describe-instances --instance-ids $stack.Stacks[0].Outputs[0].OutputValue | ConvertFrom-Json
    $instance.Reservations[0].Instances.Ipv6Address, $instance.Reservations[0].Instances.PublicIpAddress
    ping $instance.Reservations[0].Instances.Ipv6Address
    */
  }
}

export interface UtilityServerStackProps extends cdk.StackProps {
  readonly addressSubnetIndex: number;
  readonly addressSuffix: string;
  readonly availabilityZoneIndex: number;
  readonly environment?: string;
  readonly subnetType: SubnetType;
  readonly vpc?: Vpc;
}
