import * as cdk from 'aws-cdk-lib';
import { Construct } from 'constructs';
import { NetworkLayer } from './network-layer';
import { IIpAddresses } from 'aws-cdk-lib/aws-ec2';
import { CfnOutput, Tags } from 'aws-cdk-lib';

export class AwsLandingStack extends cdk.Stack {
  constructor(scope: Construct, id: string, props?: AwsLandingStackProps) {
    super(scope, id, props);

    const networkLayer = new NetworkLayer(this, "Network", {
      ipv4PrivateAddresses:  props?.ipv4PrivateAddresses,
      maxAzs: 2
    });

    Tags.of(this).add('Owner', 'IoT Lab');
    Tags.of(this).add('Classification', 'Confidential');

    new CfnOutput(this, 'VpcId', { value: networkLayer.vpc.vpcId });
    new CfnOutput(this, 'PublicSubnetIds', { value: networkLayer.vpc.publicSubnets.map(x => x.subnetId).join(',') });
    new CfnOutput(this, 'PrivateSubnetIds', { value: networkLayer.vpc.privateSubnets.map(x => x.subnetId).join(',') });
  }
}

export interface AwsLandingStackProps extends cdk.StackProps {
  readonly environment?: string;
  readonly ipv4PrivateAddresses?: IIpAddresses;
}
