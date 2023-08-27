Nordic Thingy:91 LwM2M Asset Tracker
====================================

Asset tracking full application, running on Thingy:91,
using Lightweight Machine-to-Machine (LwM2M) for the server connection.

Documentation:
* Latest: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
* Used: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.1/nrf/applications/asset_tracker_v2/README.html


Generate keys
-------------

Can be up to 32 hex characters; use the full size for best security.

```powershell
((Get-Random -Max 0xff -Count 32|ForEach-Object ToString X2) -join '')
```

Generate two keys:

1. Generate bootstrap PSK: configure this in device to connect for bootstrapping and in the server.
2. Generate device PSK: configure in server only; will be downloaded to the device during bootstrap; can be rotated as needed.

e.g.
Boostrap key: E7530493480F8D522C84AC5867B469880D5B954D1F466255B70B5D7852EB723B
Device key: 9E83F6E38B50A1C148924A14579DB25266444A3708D6E508A0B10E6620417801


Device configuration
--------------------

Hard coding and compiling security credentials into your source code is a bad idea,
so even in development, load the credentials, e.g. bootstrap pre-shared key, 
independently to the Thingy:91 device.

You will need to insert the bootstrap server into the source code (use a DNS entry,
so that it can be resolved as needed), but using bootstrap means the application server,
and the application credentials can be rotated as needed.

To configure credentials in the Thingy:91, see: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.1/nrf/applications/asset_tracker_v2/doc/cloud_wrapper.html

Also: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.1/nrf/samples/nrf9160/lwm2m_client/provisioning.html#lwm2m-client-provisioning

Credentials are stored in default security tags:

```
CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG = 35724862
CONFIG_LWM2M_CLIENT_UTILS_SERVER_TLS_TAG = 35724861
```

However, for a production system you may consider using custom tag locations.

Steps:

1. Program the nRF9160 AT Client sample to the device https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.1/nrf/samples/nrf9160/at_client/README.html#at-client-sample
   * Create new application from at_client template
   * Build for thingy91_nrf9160_ns
   * Run the nRF Connect Programmer
   * Hold down the Thingy:91 button while turning on to put in programming mode
   * Write `build/zephyr/app_signed.hex` to device
   * Restart device
2. Run the nRF Connect Serial Terminal and connect to the device
3. Identify IMEI (e.g. 350457791735879)
AT+CGSN
4. List (1) existing (type 3 = PSK, 4 = PSK ID); this only shows if they exist
AT%CMNG=1
5. Delete (3) existing bootstrap tags, if needed
AT%CMNG=3,35724862,4
AT%CMNG=3,35724862,3
5. Write (0) PSK and ID into bootstrap tags:
AT%CMNG=0,35724862,4,"urn:imei:350457791735879"
AT%CMNG=0,35724862,3,"E7530493480F8D522C84AC5867B469880D5B954D1F466255B70B5D7852EB723B"

Note you can read the ID, but not the key.
AT%CMNG=2,35724862,4

Use the same PSK (and ID) when configuring the server.

### Scripting

The nRF SDK includes a provision.py script that sets the AT commands and also provisions in the Coiote device management.

A similar script could be used with other systems, e.g. program the AT Client, run the script to generate keys and load to the device, and store in an upload file; then program the Asset Tracker client (with the bootstrap address). Later upload the file with keys to the bootstrap system.


Create new app
--------------

* Run nRFConnect for Desktop, Toolchain Manager, check SDK, open VS Code

New Application

* Type: Workspace
* nRF Connect SDK: v2.4.1
* nRF Connect Toolchain: 2.4.0
* Workspace location: /home/sly/Code/Nordic
* Workspace name: telstra_asset_tracker
* Template: nrf/applications/asset_tracker_v2
* Application name: asset_tracker_v2

Will create a .west folder and link the SDK repositories with the app.

Can also create a stand alone application.


Project configuration
---------------------

Add the boostrap server to the bottom of `overlay-lwm2m.conf`. Leaving the Pre-Shared Key (PSK) blank will use the one from the security tag (loaded to the device).

```
# Bootstrap config
CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP=y
CONFIG_LWM2M_CLIENT_UTILS_SERVER="coaps://ec2-54-206-3-178.ap-southeast-2.compute.amazonaws.com:5681"
CONFIG_LWM2M_INTEGRATION_PSK=""
```

Also configure the mobile packet data network in `prj.conf`:

```
# Packet data network
CONFIG_PDN=y
CONFIG_PDN_DEFAULTS_OVERRIDE=y
CONFIG_PDN_DEFAULT_APN="telstra.iot"
CONFIG_PDN_DEFAULT_FAM_IPV6=y
```

Build
-----

In VS Code NRF CONNECT:

* Applications > asset_tracker_v2 > Create new build configuration
* Board = thingy91_nrf9160_ns
* Build directory = build
* Kconfig fragments = overlay-lwm2m.conf, OK
* Build Configuration

Use the Programmer to write `build/zephyr/app_signed.hex` to the device, 
then turn the device off until ready to connect.


Server configuration
--------------------

Boostrap:
* Device ID: urn:imei:350457791735879
* Security Mode: PSK
* Boostrap Identity: urn:imei:350457791735879
* Boostrap Key: E7530493480F8D522C84AC5867B469880D5B954D1F466255B70B5D7852EB723B
  - Used to bootstrap; needs to be the same as the key loaded on the device

Device:
* LwM2M Identity: urn:imei:350457791735879
* LwM2M Key: 9E83F6E38B50A1C148924A14579DB25266444A3708D6E508A0B10E6620417801
  - This key can be rotated, and will be downloaded to the device as part of bootstrap

* LwM2M Server: coaps://ec2-54-206-3-178.ap-southeast-2.compute.amazonaws.com:5684
  - This will be downloaded to the device as part of bootstrap

After registration, the device will not be pending, but be added to All Devices.
Go to the last page to find the newly added device.


Starting the device
-------------------

With the nRF Connect Serial Terminal running, and the device open in Cumulocity
leave the Thingy:91 connected to the computer (via USB) and turn it on.

Logs will appear in the Serial Terminal and the light will start flashing green.


