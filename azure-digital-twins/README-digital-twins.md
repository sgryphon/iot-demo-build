Azure Digital Twins
===================

ADT has both a control plane API and data plan API.

You can use Azure CLI to access both management resources and the data plane, e.g.

```powershell
az dt list
az dt twin query -n "dt-iotcore-0x$((az account show --query id --output tsv).Substring(0,4))-dev" -q "SELECT * FROM DigitalTwins"
```

Or you can use the REST API, e.g. for the control plane (requires management token):

```powershell
$manageToken = az account get-access-token --resource "https://management.azure.com" | ConvertFrom-Json
$secureManageToken = ConvertTo-SecureString -String $manageToken.accessToken -AsPlainText
$subscription = az account show | ConvertFrom-Json
$listDigitalTwins = "https://management.azure.com/subscriptions/$($subscription.id)/providers/Microsoft.DigitalTwins/digitalTwinsInstances?api-version=2020-12-01"
$listResponse = Invoke-WebRequest $listDigitalTwins -Authentication OAuth -Token $secureManageToken
$digitalTwins = $listResponse.Content | ConvertFrom-Json
$digitalTwins.value
```

To query the data plane, you need a different token and use the host name of the twin.

```powershell
$dataToken = az account get-access-token --resource 0b07f429-9f4b-4714-9392-cc5e8e80c8b0 | ConvertFrom-Json
$secureDataToken = ConvertTo-SecureString -String $dataToken.accessToken -AsPlainText
$queryTwins = "https://$($digitalTwins.value.properties.hostName)/query?api-version=2020-10-31"
$queryBody = "{ `"query`": `"SELECT * FROM DIGITALTWINS`" }"
$queryResponse = Invoke-WebRequest $queryTwins -Method Post -Body $queryBody -ContentType "application/json" -Authentication OAuth -Token $secureDataToken
$queryResponse.Content
```

Note: The query will be empty if you have not created any twins yet.


Visual API testing
------------------

### Hoppscotch

This is an online tool, so can be used without installing anything. (For increased security, e.g. if accessing live systems, you can run it locally via docker.)

* Online tool: https://hoppscotch.io/
* Run from local docker: `docker run --rm --name hoppscotch -p 3000:3000 hoppscotch/hoppscotch:latest`
* Github repository: https://github.com/hoppscotch/hoppscotch

You can import the sample collections (or enter the REST API URLs directly yourself):

* Postman format: https://github.com/microsoft/azure-digital-twins-postman-samples
* OpenAPI / Swagger: https://github.com/Azure/azure-rest-api-specs/tree/master/specification/digitaltwins/data-plane/Microsoft.DigitalTwins

Instructions:

1. Download the Postman collection
2. Open Hoppscotch (and login if you want)
3. Select Collections (the folder) on the right
4. Select Import / Export (box) on the far right
5. Import from Postman
6. Choose file, and selet the postman_collection.json downloaded from the Microsoft samples
7. Click Import

Expand the folder to see the samples.

#### Control plane operations

You will first need to know you subscription ID, and obtain a Bearer token:

```
az account show --query id --output tsv
az account get-access-token --resource "https://management.azure.com" --query accessToken --output tsv
```

1. Open the folder Control Plane - Digital Twin Instance
2. Select Digital Twins List (replace existing query if asked)
3. Replace <<subscriptionid>> in the URL with your subscription ID
4. In the Authorization tab, select Authorization Type OAuth 2.0
5. Paste in your management access token
6. Click Send

You can also create an environment (on the right), e.g. `Digital Twins Demo` and set variables such as `subscriptionId`, which will be automatically references in the collection templates (as `<<subscriptionId>>`).

You can get the hostName from the returned digital twins list. It will look like `dt-iotcore-<orgid>-dev.api.aue.digitaltwins.azure.net`

You can set this in the environment parameters as `digitaltwins-hostname`.

#### Data plane operations

To access the data plane you will need a different token; the static resource ID is `0b07f429-9f4b-4714-9392-cc5e8e80c8b0`.

```
az account get-access-token --resource "0b07f429-9f4b-4714-9392-cc5e8e80c8b0" --query accessToken --output tsv
```

1. Open Data Plan - Twins folder
2. Select Query Twins (replace existing query if asked)
3. If you have set the `digitaltwins-hostname` environment parameter it will be used, otherwise you need to replace <<digitaltwins-hostname>> in the URL with your digital twins host name.
4. In the Authorization tab, select Authorization Type OAuth 2.0
5. Paste in your data plane access token
6. Click Send

The request should be successful, however if you have not used the digital twins yet, the response will be empty.

Note: You may need to change the interceptor, e.g. use Proxy, if the host name is not recognised.


### Advanced REST Client

* Documentation, including installation instructions: https://docs.advancedrestclient.com/
* Github repository: https://github.com/advanced-rest-client/arc-electron


Digital Twin Endpoints
----------------------

Digital Twins currently supports the following Azure services as endpoints:

* Event Grid
* Event Hubs
* Service Bus Topics

You can also create a Data History Connection via Event Hubs to Azure Data Explorer.

See: https://docs.microsoft.com/en-us/azure/digital-twins/concepts-route-events

