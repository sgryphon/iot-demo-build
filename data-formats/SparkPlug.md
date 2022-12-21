SparkPlug data format
=====================

* https://www.eclipse.org/tahu/spec/Sparkplug%20Topic%20Namespace%20and%20State%20ManagementV2.2-with%20appendix%20B%20format%20-%20Eclipse.pdf

SparkPlug is focussed on a structured MQTT format that is compatible with SCADA systems.

The system includes lifecycle management of both edge nodes and attached devices, with a grouping structure above that. There is handling for both birth messages of nodes and devices, and death messages (using MQTT will messages).


MQTT topics
-----------

Topics use the following structure: "spBv1.0/{group_id}/{message_type}/{edge_node_id}[/device_id]"

The namespace is "spBv1.0" (there is also a SparkPlug A, used for an older version).

Group ID is used for high level grouping.

Message type is one of the following: NBIRTH, NDEATH, DBIRTH, DDEATH, NDATA, DDATA, NCMD, DCMD, or STATE.

N* messages are for nodes, and D* messages are for devices. CMD is for commands sent to devices, and STATE is for notifying the system the state of the server.

Servers should listen to "spBv1.0/+/NBIRTH/#", DBIRTH, NDEATH, DDEATH, NDATA, and DDATA topics.

Edge nodes should listen to "spBv1.0/{group_id}/NCMD/{edge_node_id}/#" and DCMD topics, passing commands to devices as needed. They should also listen to "spBv1.0/+/STATE/#" messages if they need to track server state.


MQTT messages
-------------

Messages are actually binary serialized, using Google Protocol Buffers, but shown here in JSON for readabilty.

A DDATA (device data) message may look like the following:

```json
{
    "timestamp": 1486144502122,
    "metrics": [{
        "name": "Inputs/A",
        "timestamp": 1486144502122,
        "dataType": "Boolean",
        "value": true
    }, {
        "name": "Inputs/C",
        "timestamp": 1486144502122,
        "dataType": "Boolean",
        "value": true
    }],
    "seq": 0
}
```

Metrics are organised in a normalised flat list, but the names allow for hierarchical paths, used for display and grouping in the target end system. Special paths for 'Node Control' and 'Properties' are used for device control (such as triggering Reboot or Rebirth), and node or device properties (such as Hardware Model, OS Version, or Supply Voltage (V)).

Numeric aliases can be provided (1, 2, 3, etc), with the full name only given one, to simplify messages. There are several metadata attributes for things like is_historical or is_transient that allow data to be passed that does not update the current state (but may be stored).
