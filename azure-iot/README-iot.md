Azure IoT
=========

Creates the following Azure resources (in resource group `rg-iotcore-dev-001`):

* IoT Hub, `iot-hub001-<OrgId>-dev`
* Device Provisioning Service, `dps-iotcore-<OrgId>-dev`
* Digital Twins, `dt-iotcore-<OrgId>-dev`
* Connection to the Azure Data Explorer Cluster, `dec<OrgId>dev`

IoT Hub diagnostics are sent to Azure Monitor from the `azure-landing` shared services.

The IoT Hub is configured as a linked hub with DPS.

Azure Data Explorer (ADX) is configured with a database `dedb-iotcore-<OrgId>-dev`, with raw ingestion table `IotHub001-raw`, and a data connection configured to a consumer group on the hub. (The ADX cluster must be running to deploy this.)

All messages from the hub are ingested into the table as raw JSON.

Individual applications can then use ADX update policy to split and parse the specific messages they are interested in to a relevant schema table and materialized views. (This recommended approach comes from the Microsoft Azure IoT Architecture summit.)

For a quick demonstration, the raw ingestion table can also be queried directly, e.g. to build an ADX dashboard.

### Deployment

Requirements:
* PowerShell
* Azure CLI

The deployment also relies on the landing zone Azure Monitor components, so deploy that first.

Deploy via PowerShell (must have the ADX cluster running for deployment):

```powershell
az login
az account set --subscription <subscription id>
$VerbosePreference = 'Continue'
./infrastructure/deploy-iotcore.ps1
```

### Azure Data Explorer cost management


The ADX compute cluster can be shut down when not needed to save costs when running an Azure developer subscription. (You still pay for storage.) 

To shut down:

```powershell
.../azure-landing/infrastructure/stop-dataexplorer.ps1
```

### Cleanup

This will remove the resource groups, deleting all resources in them.

```powershell
./remove-iotcore.ps1
```

IoT Endpoints
-------------

Service endpoints. Each IoT hub exposes a set of endpoints for your solution back end to communicate with your devices.

* Built-in Azure Event Hubs compatible endpoint (AMQPS, or AMQP over WebSockets)
  - Receive device-to-cloud messages
  - Send cloud-to-device-messages and receive delivery acknowledgements
  - Receive file notifications
  - Direct method invocation

IoT Hub currently supports the following Azure services as additional endpoints:

* Azure Storage containers
* Event Hubs
* Service Bus Queues and Service Bus Topics

IoT Hub also integrates with and can publish event messages to:

* Azure Event Grid

See: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-endpoints
And: https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-d2c

Events
------

Event Grid vs Event Hubs vs Service Bus

https://docs.microsoft.com/en-us/azure/event-grid/compare-messaging-services


Ideas
-----

Would Synapse be an alternative to ADX?  i.e. Send IoT events to blob storage, then query them later?

