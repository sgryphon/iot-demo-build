param location string = resourceGroup().location
param nameSuffix string = uniqueString(resourceGroup().id)

// Stack configuration
@description('The Things Stack Cluster HTTP Address')
param stackClusterAddress string = 'https://tenant.region.cloud.thethings.industries/api/v3'

@description('The Things Stack Application ID')
param stackApplicationID string

@secure()
@description('The Things Stack API Key')
param stackAPIKey string

// SKU configuration
param eventHubNameSpaceSKU string = 'Standard'

param iotHubSKU string = 'S1'
param iotHubCapacity int = 1

param storageAccountSKU string = 'Standard_LRS'

param appServicePlanSKU string = 'Y1'
param appServicePlanTier string = 'Dynamic'

// IoT Hub configuration
@description('If enabled, the default IoT Hub fallback route will be added')
param enableFallbackRoute bool = true

// Resource names
var functionAppName = 'fn-${nameSuffix}'
var appServicePlanName = 'aps-${nameSuffix}'
var appInsightsName = 'ai-${nameSuffix}'
var storageAccountName = 'fnstor${replace(nameSuffix, '-', '')}'
var eventHubNamespaceName = 'evhubns-${nameSuffix}'
var iotHubName = 'iothub-${nameSuffix}'

var functionName = 'SubmitEvents'
var functionFullName = '${functionApp.name}/${functionName}'
var eventHubName = 'Events'
var eventHubFullName = '${eventHubNamespace.name}/${eventHubName}'
var eventHubEndpointName = 'StackEvents'

// Outputs
output iotHubHostname string = iotHub.properties.hostName
output iotHubOwnerKey string = iotHub.listKeys().value[0].primaryKey

// Resources
resource eventHubNamespace 'Microsoft.EventHub/namespaces@2021-01-01-preview' = {
  name: eventHubNamespaceName
  location: location
  sku: {
    name: eventHubNameSpaceSKU
  }
  properties: {}
}

resource eventHub 'Microsoft.EventHub/namespaces/eventhubs@2021-01-01-preview' = {
  name: eventHubFullName
  properties: {}
}

resource eventHubSendAuth 'Microsoft.EventHub/namespaces/eventhubs/authorizationRules@2021-01-01-preview' = {
  parent: eventHub
  name: 'Send'
  properties: {
    rights: [
      'Send'
    ]
  }
}

resource eventHubListenAuth 'Microsoft.EventHub/namespaces/eventhubs/authorizationRules@2021-01-01-preview' = {
  parent: eventHub
  name: 'Listen'
  properties: {
    rights: [
      'Listen'
    ]
  }
}

resource iotHub 'Microsoft.Devices/IotHubs@2021-03-31' = {
  name: iotHubName
  location: location
  sku: {
    name: iotHubSKU
    capacity: iotHubCapacity
  }
  properties: {
    routing: {
      endpoints: {
        eventHubs: [
          {
            name: eventHubEndpointName
            connectionString: eventHubSendAuth.listKeys().primaryConnectionString
          }
        ]
      }
      routes: [
        {
          name: 'TTSTwinChangeEvents'
          isEnabled: true
          source: 'TwinChangeEvents'
          condition: 'IS_OBJECT($body.properties.desired) OR IS_OBJECT($body.tags)'
          endpointNames: [
            eventHubEndpointName
          ]
        }
        {
          name: 'TTSDeviceLifecycleEvents'
          isEnabled: true
          source: 'DeviceLifecycleEvents'
          endpointNames: [
            eventHubEndpointName
          ]
        }
      ]
      fallbackRoute: enableFallbackRoute ? {
        name: 'FallbackRoute'
        isEnabled: true
        source: 'DeviceMessages'
        endpointNames: [
          'events'
        ]
      } : {}
    }
  }
}

resource storageAccount 'Microsoft.Storage/storageAccounts@2021-04-01' = {
  name: storageAccountName
  location: location
  sku: {
    name: storageAccountSKU
  }
  kind: 'StorageV2'
  properties: {
    supportsHttpsTrafficOnly: true
    encryption: {
      services: {
        file: {
          keyType: 'Account'
          enabled: true
        }
        blob: {
          keyType: 'Account'
          enabled: true
        }
      }
      keySource: 'Microsoft.Storage'
    }
    accessTier: 'Hot'
  }
}

resource appInsights 'Microsoft.Insights/components@2020-02-02' = {
  name: appInsightsName
  location: location
  kind: 'web'
  properties: {
    Application_Type: 'web'
    publicNetworkAccessForIngestion: 'Enabled'
    publicNetworkAccessForQuery: 'Enabled'
  }
}

resource appServicePlan 'Microsoft.Web/serverfarms@2021-01-15' = {
  name: appServicePlanName
  location: location
  kind: 'functionapp'
  sku: {
    name: appServicePlanSKU
    tier: appServicePlanTier
  }
  properties: {}
}

resource functionApp 'Microsoft.Web/sites@2021-01-15' = {
  name: functionAppName
  location: location
  kind: 'functionapp'
  properties: {
    serverFarmId: appServicePlan.id
    siteConfig: {
      appSettings: [
        {
          name: 'AzureWebJobsStorage'
          value: 'DefaultEndpointsProtocol=https;AccountName=${storageAccount.name};EndpointSuffix=${environment().suffixes.storage};AccountKey=${storageAccount.listKeys().keys[0].value}'
        }
        {
          name: 'WEBSITE_CONTENTAZUREFILECONNECTIONSTRING'
          value: 'DefaultEndpointsProtocol=https;AccountName=${storageAccount.name};EndpointSuffix=${environment().suffixes.storage};AccountKey=${storageAccount.listKeys().keys[0].value}'
        }
        {
          name: 'APPINSIGHTS_INSTRUMENTATIONKEY'
          value: appInsights.properties.InstrumentationKey
        }
        {
          name: 'APPLICATIONINSIGHTS_CONNECTION_STRING'
          value: 'InstrumentationKey=${appInsights.properties.InstrumentationKey}'
        }
        {
          name: 'FUNCTIONS_WORKER_RUNTIME'
          value: 'dotnet'
        }
        {
          name: 'FUNCTIONS_EXTENSION_VERSION'
          value: '~3'
        }
        {
          name: 'EVENTHUB_CONNECTION_STRING'
          value: eventHubListenAuth.listKeys().primaryConnectionString
        }
        {
          name: 'STACK_BASE_URL'
          value: stackClusterAddress
        }
        {
          name: 'STACK_APPLICATION_ID'
          value: stackApplicationID
        }
        {
          name: 'STACK_API_KEY'
          value: stackAPIKey
        }
      ]
    }
    httpsOnly: true
  }
}

resource function 'Microsoft.Web/sites/functions@2021-01-15' = {
  name: functionFullName
  properties: {
    config: {
      disabled: false
      scriptFile: 'run.csx'
      bindings: [
        {
          type: 'eventHubTrigger'
          name: 'events'
          direction: 'in'
          eventHubName: eventHubFullName
          connection: 'EVENTHUB_CONNECTION_STRING'
          cardinality: 'many'
          consumerGroup: '$Default'
          dataType: 'binary'
        }
      ]
    }
    files: {
      'run.csx': loadTextContent('./fns/SubmitEvents/run.csx')
      'function.proj': loadTextContent('./fns/SubmitEvents/function.proj')
    }
  }
}
