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

You should at least include the system properties "iothub-enqueuedtime" and "iothub-connection-device-id", otherwise you may not know when the message arrived or where it came from ("iothub-creation-time-utc" is also important when messages are batched).

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

Azure Synapse
-------------

Raw data files, in JSON Line format (.jsonl), contain rows of JSON.

### SQL

You can open these files as tab-separated values (using CSV format with field terminator '\t'), which will have a single column, and then use `JSON_VALUE()` to extract out JSON fields as columns.

```sql
SELECT
    TOP 100 
        JSON_VALUE(rawevent, '$.EnqueuedTimeUtc') as EnqueuedTimeUtc, 
        JSON_VALUE(rawevent, '$.SystemProperties.connectionDeviceId') as DeviceId,
        JSON_VALUE(rawevent, '$.Body.voltage') as Voltage,
        JSON_VALUE(rawevent, '$.Body.currentTemperature') as CurrentTemperature,
        rawevent
FROM
    OPENROWSET(
        BULK 'https://straw0xacc5dev001.dfs.core.windows.net/landing/Landing/Telemetry/iot-hub001-0xacc5-dev/**',
        FORMAT = 'CSV',
        FIELDTERMINATOR = '\t',
        FIELDQUOTE = '\0',
        MAXERRORS = 1000,
        PARSER_VERSION = '1.0'
    )
    WITH ( [rawevent] VARCHAR(MAX) )
    AS [result]
ORDER BY EnqueuedTimeUtc DESC
```

### Spark Notebook

A notebook can also be used to load, query, and plot the data, using a Spark serverless cluster.

```python
%%pyspark
import matplotlib.pyplot as plt
from pyspark.sql.functions import *

events_df = spark.read.option("recursiveFileLookup", True)\
    .json('abfss://landing@straw0xacc5dev001.dfs.core.windows.net/Landing/Telemetry/iot-hub001-0xacc5-dev/01/2022/07/31/**')
history_df = events_df.select(to_timestamp(col("EnqueuedTimeUtc")).alias("EnqueuedTime"), "SystemProperties.connectionDeviceId", "Body.voltage", "Body.currentTemperature")
#df3 = history_df.select(to_timestamp(col("EnqueuedTimeUtc")))
#history_df.printSchema()
#display(history_df.limit(5))
history_pd_df = history_df.where("EnqueuedTime > current_timestamp - interval 2 hours").toPandas()
#history_pd_df.printSchema()

ax1 = history_pd_df['voltage'].plot()
ax1.set_title('Battery last 2 hours')
ax1.set_xlabel('Sample')
#ax1.set_xticks()
ax1.set_ylabel('Voltage')

#plt.xticks(range(len(history_pd_df['EnqueuedTime'])), history_pd_df['EnqueuedTime'])
plt.suptitle('')
plt.show()
```

Azure Data Explorer
-------------------

Query raw data:

```kusto
['IotHub001-raw'] | take 10
```

Convert raw data and extract some values:

```kusto
['IotHub001-raw']
| where tostring(rawevent.uplink_message.version_ids.model_id) == 'ldds75'
| project
    ReceivedAt = todatetime(rawevent.uplink_message.received_at),
    DeviceId = tostring(rawevent.end_device_ids.device_id),
    TtnApplication = tostring(rawevent.end_device_ids.application_ids.application_id),
    ModelId = tostring(rawevent.uplink_message.version_ids.model_id),
    DistanceM = toreal(rawevent.uplink_message.decoded_payload.distance_metres),
    BatteryV = toreal(rawevent.uplink_message.decoded_payload.battery_volts),
    RawData = base64_decode_toarray(tostring(rawevent.uplink_message.frm_payload))
| take 10
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

