#!/usr/bin/env node
import 'source-map-support/register';
import * as cdk from 'aws-cdk-lib';
import { AwsLandingStack } from '../lib/aws-landing-stack';
import { IpAddresses, SubnetType, Vpc } from 'aws-cdk-lib/aws-ec2';
import { UtilityServerStack } from '../lib/utility-server-stack';

const app = new cdk.App();
const landingNetworkDev = new AwsLandingStack(app, 'AwsLandingStack-dev', {
  /* If you don't specify 'env', this stack will be environment-agnostic.
   * Account/Region-dependent features and context lookups will not work,
   * but a single synthesized template can be deployed anywhere. */

  /* Uncomment the next line to specialize this stack for the AWS Account
   * and Region that are implied by the current CLI configuration. */
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },

  /* Uncomment the next line if you know exactly what Account and Region you
   * want to deploy the stack to. */
  // env: { account: '123456789012', region: 'us-east-1' },

  /* For more information, see https://docs.aws.amazon.com/cdk/latest/guide/environments.html */
  ipv4PrivateAddresses: IpAddresses.cidr('10.1.0.0/21'),
  environment: 'Dev',
});

const landingNetworkTest1 = new AwsLandingStack(app, 'AwsLandingStack-test1', {
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },
  ipv4PrivateAddresses: IpAddresses.cidr('10.96.0.0/21'),
  environment: 'Test',
});

const landingNetworkTest2 = new AwsLandingStack(app, 'AwsLandingStack-test2', {
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },
  ipv4PrivateAddresses: IpAddresses.cidr('10.97.0.0/21'),
  environment: 'Test',
});

new UtilityServerStack(app, 'UtilityServer-Public-dev', {
  env: { account: process.env.CDK_DEFAULT_ACCOUNT, region: process.env.CDK_DEFAULT_REGION },
  addressSubnetIndex: 0,
  addressSuffix: "100d",
  availabilityZoneIndex: 0,
  environment: 'Dev',
  subnetType: SubnetType.PUBLIC,
  vpc: landingNetworkDev.networkLayer.vpc
});
