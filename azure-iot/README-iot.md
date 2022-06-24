Azure IoT
=========

Creates the following Azure resources:

* IoT Hub, `iot-hub001-<OrgId>-dev`
* Device Provisioning Service, `dps-iothub-<OrgId>-dev`
* Data Explorer Cluster, `dec<OrgId>dev`

IoT Hub diagnostics are sent to Azure Monitor from the `azure-landing` shared services.

The IoT Hub is configured as a linked hub with DPS.

Azure Data Explorer (ADX) is configured with a database `dedb-iothub-<OrgId>-dev`, with raw ingestion table `IotHub001-raw`, and a data connection configured to a consumer group on the hub.

All messages from the hub are ingested into the table as raw JSON.

Individual applications can then use ADX update policy to split and parse the specific messages they are interested in to a relevant schema table and materialized views. (This recommended approach comes from the Microsoft Azure IoT Architecture summit.)

For a quick demonstration, the raw ingestion table can also be queried directly, e.g. to build an ADX dashboard.

### Deployment

Requirements:
* PowerShell
* Azure CLI

The deployment also relies on the landing zone Azure Monitor components, so deploy that first.

Deploy via PowerShell:

```powershell
az login
az account set --subscription <subscription id>
$VerbosePreference = 'Continue'
./infrastructure/deploy-iot.ps1
```

### Azure Data Explorer cost management

The ADX compute cluster can be shut down when not needed to save costs when running an Azure developer subscription. (You still pay for storage.)

To shut down:

```powershell
./infrastructure/stop-dataexplorer.ps1
```

To restart when needed:

```powershell
./infrastructure/start-dataexplorer.ps1
```

### Cleanup

This will remove the resource groups, deleting all resources in them.

```powershell
./remove-iot.ps1
```
