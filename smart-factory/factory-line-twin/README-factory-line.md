Factory Line Twin
=================

As a factory maintainer, I want to know when machines are operating strange, so that we can undertake preventative maintenance. (Digital Twins)

Features:
* Time series of machine readings for anomally detection.
* Factory visualisation for patterns and relationships.

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


SAREF Manufacturing
-------------------

In order to locate the ProductionEquipment, a factory layout can be created. A factory is represented by the
s4inma:Factory class (which is subclass of the s4bldg:Building class) and can be further divided into
smaller spaces using the s4bldg:BuildingSpace class.

For example, the instantiation of SAREF4INMA in clause 4.3
defines the CuttingFunction, FormingFunction and JoiningFunction subclasses, which describe functions that can be
performed by different types of production equipment, such as LaserCuttingMachine, WeldingMachine,
MillingMachine, MouldingMachine and StampingMachine.


s4inma:WorkCenter

A subclass of s4inma:ProductionEquipment (and therefore of
s4bld:PhysicalObject). It is an equipment element under an area in a role-
based equipment hierarchy that performs production, storage or material
movement (definition taken from IEC 62264 [i.11]). An Area contains work
centers.

Ex: Production line (for equipment), Storage room (for temp)

s4inma:ProductionEquipment

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






Alignment to W3C Building Topology Ontology
-------------------------------------------

BOT is used in the Microsoft SmartBuildings DTDL based on RealEstateCore.

| Subject | Predicate | Object |
| ------- | --------- | ------ |
| bot:Building | owl:equivalentClass | saref4bldg:Building |
| bot:Element | owl:equivalentClass | saref4bldg:PhysicalObject |
| bot:Space | owl:equivalentClass | saref4bldg:BuildingSpace |
| bot:hasSpace | rdfs:subPropertyOf | saref4bldg:hasSpace |
| bot:containsElement | owl:equivalentProperty | saref4bldg:contains |
