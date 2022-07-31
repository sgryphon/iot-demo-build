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
./deploy-storage.ps1
./deploy-shareddata.ps1
```

### Cleanup

This will remove the resource groups, deleting all resources in them.

```powershell
./remove-shareddata.ps1
./remove-storage.ps1
./remove-shared.ps1
./remove-network.ps1
```

Components
----------

The landing zone template has shared (spoke) network and services.

### Networking resource group (rg-network-dev-001)

* Single vnet (vnet-dev-australiaeast-001)
  - IPv6 Unique Local Address network, with a consistent global ID, and subnet IDs
  - Two subnets:
    - A core (internal) network (snet-core-dev-australiaeast-001)
    - A de-militarized zone (DMZ) for Internet connected resources (snet-dmz-dev-australiaeast-001)
  - Azure networks must be dual stack, so it also has a matching 10.x.0.0/16 network and 10.x.y.0/24 subnets
* Two network security groups, for the two subnets (nsg-core-dev-001, nsg-dmz-dev-001)
  - The DMZ security group allows incoming SSH, ICMP, and web (HTTP and HTTPS) traffic

### Shared services resource group (rg-shared-dev-001)

* Azure Monitor (log-shared-dev)
  - Application Insights (appi-shared-dev)
* KeyVault (kv-shared-<OrgId>-dev)

### Storage resource group for data (rg-storage-dev-001)

Uses three storage accounts for different quality of data, with retention policies to move to cold storage after 90 days.

* Raw (Bronze) storage account (straw<OrdId>dev001)
  - `landing` container, with top level folder structure
  - `conformance` container, with top level folder structure
* Enriched and Curated storage account (stencur<OrdId>dev001)
  - `standardized` container for enriched (Silver) data, with top level folder structure
  - `data-products` container for curated (Gold) data
* Workspace storage account (stwork<OrdId>dev001)
  - `analytics-sandbox`, for development and testing
  - `synapse-primary-storage`, as a workspace for Synapse

For more information see:

* https://docs.microsoft.com/en-us/azure/cloud-adoption-framework/scenarios/cloud-scale-analytics/best-practices/data-lake-zones 

### Shared data services resource group (rg-shared-data-dev-001)

* Azure Data Explorer (ADX) Cluster, `dec<OrgId>dev`, for hot path time series analysis
  - ADX is expensive to keep running, so the script stops it after creation
  - There are scripts to start and stop as needed
* Azure Synapse, for cold/warm path analysis, pipelines/integration, reporting, and machine learning
  - Linked to the workspace `synapse-primary-storage`
  - Has built in serverless SQL pool.
  - Spark serverless pool `sparkpool001` created, with auto-pause after 15 minutes.
  - No dedicated pools (would be expensive).

Note:

* ADX has a direct connection from IoT Hub, et up in the IoT core section.
* Synapse reads from the data storage accounts, which IoT Hub is set up to write to.


Cost management
---------------

The ADX compute cluster can be shut down when not needed to save costs when running an Azure developer subscription. (You still pay for storage.)

To shut down:

```powershell
./infrastructure/stop-dataexplorer.ps1
```

To restart when needed:

```powershell
./infrastructure/start-dataexplorer.ps1
```


To Do
-----

* Apply resource locks to landing zone groups to prevent direct delete.
