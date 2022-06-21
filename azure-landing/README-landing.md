Azure Landing Zone infrastructure
=================================

Landing zones are a way to organise the foundations of Azure subscriptions.

An organisation may have a Connectivity subscription, in a Platform management group,
with regional hub network(s), and platform services such as DNS, Azure Firewall, and Express Route.

Landing zone subscriptions are then organised under business units, with
each landing zone having some foundation componets such as a spoke virtual network,
and shared services such as Azure Monitor and KeyVault.

Workloads are then deployed into resource groups within the landing zone subscriptions.

There may be landing zone subscriptions for both production and non-production workloads.

An organisation may also have sandbox subscriptions (for investigating new features).

See more: https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/ready/landing-zone/


Usage
-----

Requirements:
* PowerShell
* Azure CLI

Deploy via PowerShell:

```powershell
az login
az account set --subscription <subscription id>
$VerbosePreference = 'Continue'
./deploy-network.ps1
./deploy-shared.ps1
```

### Cleanup

This will remove the resource groups, deleting all resources in them.

```powershell
./remove-network.ps1
./remove-shared.ps1
```

Components
----------

The landing zone template has shared (spoke) network and services.

* Networking resource group (rg-network-dev-001)
  - Single vnet (vnet-dev-australiaeast-001)
    - IPv6 Unique Local Address network, with a consistent global ID, and subnet IDs
    - Two subnets:
      - A core (internal) network (snet-core-dev-australiaeast-001)
      - A de-militarized zone (DMZ) for Internet connected resources (snet-dmz-dev-australiaeast-001)
    - Azure networks must be dual stack, so it also has a matching 10.x.0.0/16 network and 10.x.y.0/24 subnets
  - Two network security groups, for the two subnets (nsg-core-dev-001, nsg-dmz-dev-001)
    - The DMZ security group allows incoming SSH, ICMP, and web (HTTP and HTTPS) traffic
* Shared services resource group (rg-shared-dev-001)
  - Azure Monitor (log-shared-dev)
    - Application Insights (appi-shared-dev)
  - KeyVault (kv-shared-<OrgId>-dev)
