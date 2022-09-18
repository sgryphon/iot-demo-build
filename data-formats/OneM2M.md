OneM2M data format
==================

References:
* https://onem2m.org/
* https://onem2m.org/using-onem2m/developers/device-developers

Allows cross domain collaboration through a set of common services, including device management, registration, and security.

The system is layered, with an application layer (the specific functions, such as temperature measurement feeding into a building management system), the common services layer (OneM2M), and below that a connectivity / network layer.

The terminology is a bit complicated, but essential a device is a node and can have all of the application layer, common layer, and connectivity layer running on that device (but not necessarily). Device nodes are deployed in the field domain (compared to the central servers in the infrastructure domain).

The application layer component is called an Application Entity (AE), and the common layer component is called a Common Services Entity (CSE). Both of these can be on a device node, but in some cases they may be separate, e.g. the AE is on a dumb sensor an the CSE on a connected data logger.

The primary transport mechanism are REST architectures such as HTTP and CoAP.

Message (request and response) primities are transport agnostic and consist of a control part and a content part. The control part is encoded based on the protocol binding (e.g. headers for a HTTP post), and the content part is serialised using JSON, CBOR, or XML.

For example, with HTTP

```
POST /CSE01Base?rcn=1 HTTP/1.1
Host: m2msp1.com
From: ae01.com
X-M2M-RI: 0001
X-MSM-RVI: 2a

{
    "m2m:cnt": {
        "cbs": 0,
        "cni": 0,
        "ct": "20180406T085712",
        "lt": "20180406T085712",
        "pi": "CAE0120180406T084690_cse01",
        "ri": "cnt20180406T08571214_cse01",
        "rn": "cont_temp",
        "st": 0,
        "ty": 3
    }
}
```

| oneM2M item | Short name | HTTP binding | Value
| ----------- | ---------- | ------------ | -----
| Operation   | op         | Method       | POST
| To          | to         | Host + path  | msmsp1.com/CSE01Base 
| Result content | rcn | Query string | ?rcn=1
| From | fr | Header | ae01.com
| Request identifier | rqi | Header X-M2M-RI | 0001
| Release Version Inidicator | rvi | Header X-M2M-RVI | 2a
| Primitive Content | pc | Content | e.g. JSON

OneM2M is REST-based, so is made up of individually addressable resources. Each resource has its own specific type, with mandatory attributes, optional attributes, and child resources.

The root of the OneM2M resource structure is <CSEBase>, representing one CSE and the root for all resources in that CSE. It is assigned an absolute address. Child resources are addressed relative to <CSEBase> and may exist multiple times, e.g. you might have multiple <accessControlPolicy> children.

An <AE> resource represents and Application Entity registered to a CSE (there may be one, or multiple), and is hte root resource for all child resources of the Application Entity.

Each resource has universal attributes (all resources), common attributes (reused across resources), resource-specific attributes, and child resources.

| Universal attribute | Short name
| ------------------- | ----------
| resourceType        | ty
| resourceID          | ri
| parentID            | pi
| lastModifiedTime    | lt
| creationTime        | ct
| resourceName        | rn


Addressing
----------

Entity addressing for Service Provider (SP, a domain name), CSE (Common Services Entity), and AE (Application Entity).

EXAMPLE: 
* absolute-CSE-ID = "//myoperator.com/cse1"
Where "//myoperator.com" is the M2M-SP-ID and "/cse1" is the SP‑relative-CSE-ID.

EXAMPLES (AE-ID can be issued by SP (starts with "S") or CSE (starts with "C")): 
* S-AE-ID-Stem = "S563423" (can be used by itself)
* absolute-AE-ID = "//myoperator.com/S563423", using the S-ID
* absolute-AE-ID = "//myoperator.com/cse2/C3532ea3", using the CSE ID plus a C-ID

OneM2M identifiers are case sensitive (domain portion must always be lowercase).

Resource addressing is relative to the CSE, and can either be hierachical structured, e.g. "-/xxxx/yyyy", or an unstructured CSE-relative identifier. For absolute or SP relative addresses, the relevant prefixes are added, e.g. "//myoperator.com/cse1/-/xxxx/yyyy".

Resource names are arbitrary strings, e.g. "myLightBulb", "123Sensor"

Node IDs & Device IDs use URN format, e.g. urn:imei:123456..... 



MQTT transport
--------------

The topic structure to use is:
* For requests "/oneM2M/req/{Originator-ID}/{Receiver-ID}/{type}"
* For responses "/oneM2M/resp/{Originator-ID}/{Receiver-ID}"

{originator} is the SP-relative-AE-ID or SP-relative-CSE-ID, omitting any leading "/" and replacing other "/" with ":". For an Absolute-CSE-ID, replace the leading "//" with ":"

{receiver} is similar

{type} is either xml, json, or cbor, for the payload type

To listen for requests, the receiver subscribes to "/oneM2M/req/+/{receiver}/{type}", or a wildcard to listen for all types, i.e. "/oneM2M/req/+/{receiver}/#"

For responses, {originator} is who sent the original request, so to listen for responses the originator subscribes to "/oneM2M/resp/{originator}/#"

Initial registration requests may not know their ID, so use "/oneM2M/reg_req/{originator credential}/{receiver}", and "/oneM2M/reg_resp/..."

As there are no special headers in MQTT, the message is made up of the oneM2M control parameters at the top, then the `pc` element containing the primitive content.

```json
{
    "op": 1,
    "to": "//xxxxx/2345",
    "fr": "//xxxxx/99",
    "rqi": "A1234",
    "ty": 18,
    "pc": {
        "m2m:sch": {
            "rn":"schedule1",
            "se": {
                "sce": ["* 0-5 2,6,10 * * * *"]
            }
        }
    },
    "ot": 20150910T062032
}
```

Data
----

Enumerations:

m2m:resourceType :- 2 = AE, 3 = container, 4 = contentInstance, 12 = mgmtCmd, 13 = mgmtObj, 14 = node, 18 = schedule, 23 = subscription, 28 = flexContainer, 29 = timeSeries, 30 = timeSeriesInstance

m2m:operation :- 1 = Create, 2 = Retrieve, 3 = Update, 4 = Delete, 5 = Notify

m2m:cmdType :- 1 = Reset, 2 = Reboot, 3 = Upload, 4 = Download, 5 = SoftwareInstall, 6 = SoftwareUninstall, 7 = SoftwareUpdate

m2m:primitiveContent

m2m:batchNotify

m2m:notification

m2m:flexContainerResource
   @resourceName
   resourceType
   resourceID
   parentID
   creationTime
   lastModifiedTime
   labels
   stateTag
   containerDefinition
   contentSize

m2m:requestPrimitive
   Operation : m2m:operation
   To : xs:anyURI
   From : m2m:ID
   Request Identifier : m2m:requestID
   Resource Type : m2m:resourceType
   Content : m2m:primitiveContent
   Release Version Indicator : m2m:releaseVersion


JSON request message example
----------------------------

An example of a request message serialized using JSON is given below:

```json
{
	"op": 1,
	"fr": "Clight_ae1",
	"to": "/homegateway/light",
	"rqi": "A1234",
	"pc": {
		"m2m:sch": {
			"se": {
				"sce": ["* 0-5 2,6,10 * * * *"]
			}
		}
	},
	"ty": 18
}
```

* op: operation (in this case it is Create)
* fr: ID of the Originator (an AE in this example)
* to: URI of the target resource
* rqi: request identifier (this is a string)
* pc: attributes of the <schedule> resource, with member name "m2m:sch", as provided by the Originator. This is serialized as a nested JSON object
* ty: type of resource to be created (in this case a Schedule resource). This is a number.

Note that the Operation (op) parameter is present only in Request primitives. The presence of this parameter in JSON serialized primitive representations allows to differentiate Request primitives from Response primitives.

Response message examples
-------------------------

Response after requesting a top level AE resource, with result content "attributes and child resource references" and filter criteria level 2 (showing links of depth 1 and 2 below the AE):

```json
{ 
 "m2m:ae": { 
   "rn": "appname", 
   "aei": "CAE01", 
   "ct": "20160404T132648", 
   "et": "20160408T004648", 
   "lt": "20160404T132648", 
   "pi": "ONET-CSE-02", 
   "ri": "REQID1", 
   "ty": 2,
   "ch": [{"nm":"container1", "typ":3,  "val":"mn-cse/appname/container1"},
          {"nm":"container2", "typ":3,  "val":"mn-cse/appname/container2"},
 		  {"nm":"sub1", "typ":23, "val":"mn-cse/appname/container2/sub1"}] 
 } 
}
```

Similiar response for a request with result content "attributes and child resources", which shows the child resources inline.

```json
{ 
 "m2m:ae": { 
   "rn": "appname", 
   "aei": "CAE01", 
   "ct": "20160404T132648", 
   "et": "20160408T004648", 
   "lt": "20160404T132648", 
   "pi": "ONET-CSE-02", 
   "ri": "REQID1", 
   "ty": 2,
   "m2m:cnt":[{"rn":"container1", "ty":3,  …},
              {"rn":"container2", "ty":3,  … ,
 	 		   "m2m:sub":[{"rn":"sub1", "ty":23, …}]}
			 ]
  } 
}
```


Specific resources
----------------

memory, see D.4 of oneM2M TS-0001

"m2m:memory": {
    "mgmtDefinition": 1003,
    "memAvailable": 3123000,
    "memTotal": 4000000
}

battery, see D.7 of oneM2M TS-0001

"m2m:battery": {
    "mgmtDefinition": 1006,
    "batteryLevel": 95,
    "batteryStatus": 1
}

deviceInfo, see D.8 of oneM2M TS-0001

"m2m:deviceInfo": {
    "mgmtDefinition": 1007,
    "deviceLabel": "1234567890" OR "|systemID:0123 serviceID:xyz" OR "urn:imei:1234567890, ; serial number, key/value pairs, URNs
    "manufacturer": "M5Stack",
    "model": "M5 Atom Lite",
    "deviceType": "",
    "fwVersion": "",
    "swVersion": "",
    "hwVersion": ""
}

"m2m:reboot": {
    "mgmtDefinition": 1009,
    "reboot": true,
    "factoryReset": true
}

Update operation: if reboot is true, then reboot; if factoryReset is true, then reset; if both are set then error BAD_REQUEST

"m2m:eventLog": {
    "mgmtDefinition": 1010,
    "lotTypeId": 1,
    "logData": "",
    "logStatus": 1,
    "logStart": true,
    "logStop": true
}

Update start/stop to start/stop. Retrieve to get the current data.


Example sequence
----------------

See TR-0017 Home domain abstract model, p. 35 - 37 for an example sequence of a washer (AE) 


Home Devices
------------

deviceModelResource : <flexContainer>
  DeviceClass ID = "org.onem2m.home.device.tv"
  nodeLink = device node
  children:
    moduleResource : <flexContainer>
      ModuleClass ID = "org.onem2m.home.moduleclass.audiovolume"
      attributes = Properties of module, prefixed with 'prop'
      attributes = DataPoints
      children:
        Action : <flexContainer>
          Action ID = "org.onem2m.home.moduleclass.audiovolume.upvolume"
    subDevice: <flexContainer>

<node> (for the device)
  children:
    <mgmtObj>, e.g. firmware
    deviceInfo
      attributes = Properties of device model
  



LwM2M interworking
------------------

The OneM2M system includes a standard for interworking with LwM2M.



