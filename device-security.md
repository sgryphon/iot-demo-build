Device Security
===============

Consumer devices
----------------

* Owned by individual consumers.
* May connect locally, or to a cloud platform (run by the manufacturer).
* Have some kind of physical reset. (But may wipe previous information)
  - This allows a new owner to put the device back into provisioning mode.
* Provisioning mode usually has some kind of access point setup where a consumer can use a device, such as a phone, to connect to them.
  - For example, can be Bluetooth, or a WiFi access point or peering network.
  - The device won't yet know your WiFi details, so won't be able to connect itself.
  - Usual objective is to providie WiFi details to the device; could be via an app or Web UI served from the device.
* If the device plugs into a physical LAN, it could use DHCP to get a connection automatically.
* Security is limited.
  - Primarily via physical access to the device to reset.
  - Could have some kind of security key or 2D barcode (as a pre-shared key), but that could also be physically vulnerable (if on the device), or could become lost/unreadable (rendering the device unusable unless there is also a physical reset for that).
  - Keys could be recorded by the manufacturer, against serial number, and retrievable via validation.
* For local connection, the provisioning can also set up trust-on-first-use security (unless there is a pre-shared key, see above) 
* For manufacturer connection, provioning can be limited to wifi setup, and the onboard credentials used to validate the manufacturer (e.g. public key)
  - Even then if you can get access to the programming interface on the physical device, you could overwrite the key, or just pull data out.
  - Some devices have anti-tamper that after initial load connecting to the programming interface wipes the firmware/disables the device.

Commercial devices
------------------

* Usually a lot more security and anti-tamper.
* Factory provisioning allows fixed security to be provisioned, e.g. public key of a bootstrap server and even individual device certificates.
* Can't change owners in the field.
* NB-IoT works similar to a physical LAN, as the SIM will connect to a carrier / Access Point Name, and be on the network -- except it is a global network (not local), so needs a well known endpoint (such as a bootstrap)
* LoRaWAN is also similar -- there are limited networks, so an end device can just survey the gateways in range to find if it's PSK is registered with any networks.
* Note that consumer NB-IoT or LoRaWAN devices could work similarly.
* Commercial WiFi devices may still need some local configuration of the WiFi connection details.
* Similar options available as for consumer devices (e.g. WiFi AP mode), but with security possible.
  - Commercial provisioning also possible to have provisioning via USB serial.
    - USB serial has the same security considerations as one-time WiFi AP via physical reset, i.e. physical access.
  - For additional security only trust provisioning signed by a provisioning key (whose public key is factory written to the device).
  - LwM2M has a smart card provisioning process that works similar.

Example LoRaWAN device: The Dragino PSK is printed on a label on the box lid. If you ever lose it and need to change, you need to open the device up to get to the programming interface, and can use AT serial commands to change.


### Plan for prototype devices

* WiFi Access Point mode, via physical reset.
* Use REST API for one-time setting provisioning details (need physical reset to change).
* Provisioning public key written on device used to validate provisioning.

