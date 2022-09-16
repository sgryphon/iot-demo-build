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


MQTT transport
--------------

The topic structure to use is:
* For requests "/oneM2M/req/{Originator-ID}/{Receiver-ID}/{type>"
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

LwM2M interworking
------------------

The OneM2M system includes a standard for interworking with LwM2M.



