Factory Line Twin
=================

As a factory maintainer, I want to know when machines are operating strange, so that we can undertake preventative maintenance. (Digital Twins)

Features:
* Time series of machine readings for anomally detection.
* Factory visualisation for patterns and relationships.

The chocolate factory is based partly on the digital twins sample hands on lab https://github.com/Azure-Samples/digital-twins-samples/tree/main/HandsOnLab

The scope includes using digital twins to answer questions such as:

* Find all time windows when temperature during roasting is >65°C in the previous 24 hours and trace back events in ADT leading to that.
* Calculate the average Grinding vibration in the last 2 minutes to ensure the process meets manufacturing quality standards.
* Find all incidents with unusually higher than normal molding temperature in the previous 5 days.


Ontology: SAREF Manufacturing
-----------------------------

The Smart Applications Reference ontology (SAREF) Manufacturing extension is used as the basis for the digital twins.

SAREF Manufacturing has concepts such as:

**s4inma:ProductionEquipment**

A production equipment is a specialization of a saref:Device and
s4bldg:PhysicalObject that can produce items in a manufacturing process.
This class represents an individual production equipment machine and
includes their specification in terms of functions, states and services.
Different types of machines can be defined under this class as needed, for
example, LaserCuttingMachine (i.e. a type of production equipment to cut
steel material), MillingMachine (i.e. to drill holes in steel material),
MouldingMachine (i.e. to mould liquid material, such as iron or plastic, and
let it harden in a certain shape), WeldingMachine (i.e. to join together parts
of material, such as steel), etc.

**s4inma:WorkCenter**

A subclass of s4inma:ProductionEquipment (and therefore of
s4bld:PhysicalObject). It is an equipment element under an area in a role-
based equipment hierarchy that performs production, storage or material
movement (definition taken from IEC 62264 [i.11]). An Area contains work
centers.

**s4inma:Factory**

In order to locate the ProductionEquipment, a factory layout can be created. A factory is represented by the
s4inma:Factory class (which is subclass of the s4bldg:Building class) and can be further divided into
smaller spaces using the s4bldg:BuildingSpace class.

**s4inma:ProductionEquipmentFunction**

A specialisation of saref:Function, this allows functions to be assigned to s4inma:ProductionEquipment.

For example, the instantiation of SAREF4INMA in clause 4.3
defines the CuttingFunction, FormingFunction and JoiningFunction subclasses, which describe functions that can be
performed by different types of production equipment, such as LaserCuttingMachine, WeldingMachine,
MillingMachine, MouldingMachine and StampingMachine.

**s4inma:Measurement**

The standard also has a specialisatoin of saref:Measurement, with saref:FeatureOfInterest being an s4inma:Batch (or s4inma:Item),
i.e. allowing measurements to be related to specific batches being produced, in the broader semantic model.

However, these are telemetry and so do not exist as digital twins in the model, which focusses on the equipment.


### Deploying the ontology

The model files (core, SAREF Manufacturing, and chocolate factory specific; but not measurement) can be loaded into Azure Digital Twins.

You can do this via a PowerShell script (or manually):

```powershell
az login
$VerbosePreference = 'Continue'
./import-models.ps1
```

Once loaded you can see the model in the Digital Twins Explorer.

To replace an individual model manually, e.g. if you are making changes:

```
$dtName = "dt-iotcore-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
az dt model delete --dt-name $dtName --dtmi "dtmi:digitaltwins:s4inma:ProductionLine;1"
az dt model create --dt-name $dtName --models "Ontology/Factory/ProductionLine.json"
```

### Adding twins

There is also a script to load some sample twin instances, with one factory area containing two chocolate production lines.

```powershell
$VerbosePreference = 'Continue'
./add-twins.ps1
```

Once loaded you can see the twins in the Digital Twins Explorer.

### Clean up

To delete all twins and models:

```powershell
$dtName = "dt-iotcore-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
az dt twin delete-all --dt-name $dtName
az dt model delete-all --dt-name $dtName
```

### Alignment to W3C Building Topology Ontology

The W3C Building Topology Ontology (BOT) is used in other ontologies such as the Microsoft SmartBuildings DTDL based on RealEstateCore,
and has slightly different models for buildings and spaces. This table shows the mapping between the SAREF Building ontology and BOT.

| Subject | Predicate | Object |
| ------- | --------- | ------ |
| bot:Building | owl:equivalentClass | saref4bldg:Building |
| bot:Element | owl:equivalentClass | saref4bldg:PhysicalObject |
| bot:Space | owl:equivalentClass | saref4bldg:BuildingSpace |
| bot:hasSpace | rdfs:subPropertyOf | saref4bldg:hasSpace |
| bot:containsElement | owl:equivalentProperty | saref4bldg:contains |



TODO / Design notes
-------------------

Ideas:

* High level flow: Device -> Azure Iot Hub -> Azure Digital Twins
* Visualisation... do we need ADX?

* SAREF Manufacturing ontology

* First device: simulator on M5Stack
  - basic internal temp
  - simulated measurements (speed) with 4dF random variation
  - controls for target up/down, with gradual change
  - for Atom, have button = random spike

* Model factory:
  - devices with sensors pointed at model
  - model (lego) is independent, e.g. speed control, and sensor takes actual reading.

Plan:

1. Get temperature value from M5Stack to IoT Hub using UI Flow - at least to see what capabilities are / messages look like.
2. ?? Enhance UI flow with simulated 4dF measurements (or maybe better to switch to a language)
3. Arduino probably first (has Azure support); capture temperature; just wifi.
4. Arduino simulated factory machine (speed, vibration, etc).
5. Build twins model.

