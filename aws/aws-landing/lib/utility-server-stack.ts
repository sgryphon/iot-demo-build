import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { InstanceClass, InstanceSize, InstanceType, SubnetSelection, SubnetType, Vpc } from 'aws-cdk-lib/aws-ec2';
import { CfnOutput, Tags } from 'aws-cdk-lib';
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

    this.server = new UtilityServer(this, 'Server', {
      instanceType:  InstanceType.of(InstanceClass.T3, InstanceSize.MICRO),
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
  readonly availabilityZoneIndex: number;
  readonly environment?: string;
  readonly subnetType: SubnetType;
  readonly vpc?: Vpc;
}
