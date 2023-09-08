#!/usr/bin/env node
import 'source-map-support/register';
import * as cdk from 'aws-cdk-lib';
import { Lwm2mDemoNetworkStack } from '../lib/lwm2m-demo-network-stack';
import { Lwm2mDemoServerStack } from '../lib/lwm2m-demo-server-stack';
import { InstanceClass, InstanceSize, InstanceType, IpAddresses } from 'aws-cdk-lib/aws-ec2';

const app = new cdk.App();

const demoNetwork = new Lwm2mDemoNetworkStack(app, 'Lwm2mDemoNetworkStack', {
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },
  ipv4PrivateAddresses: IpAddresses.cidr('10.192.0.0/20'),
});

new Lwm2mDemoServerStack(app, 'Lwm2mDemoServerStack', {
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },
  addressSuffix: "100d",
  instanceType: InstanceType.of(InstanceClass.T3, InstanceSize.MICRO),
  keyName: 'leshan-demo-key',
  vpc: demoNetwork.vpc
});
