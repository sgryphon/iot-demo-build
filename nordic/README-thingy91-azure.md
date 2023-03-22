Nordic Thingy:91 Azure IoT Hub Sample
=====================================

Walkthrough setting up the Nordic Thingy:91 Azure IoT Hub sample, including fully scripted creation of
development certificates, and including IPv6-only example (using NAT64).

Main sample code: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/azure_iot_hub/README.html

1. Setup of development certificate hierarchy
2. Generate device certificates
3. Load certificates to the device
4. Run the sample
5. Running in IPv6-only network

### Toolchain setup

You can manage Nordic development using the [nRF Connect for Desktop manager](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop)

If you don't have it already, first download and install [VS Code](https://code.visualstudio.com/).

On Ubunut Linux the package is called 'code' and can be installed with `sudo snap install --classic code`. On Windows you 
can use `winget install Microsoft.VisualStudioCode -e`.

Then [Download and Install nRF Connect](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop/Download?lang=en#infotabs)
to your computer, and use it to install the Toolchain Manager, the Programmer, and other tools as needed

<pic - nRF desktop>

Then use the Toolchain Manager to install the required version of the SDK; at the time of this article it was v2.3.0. This will also
install the required VS Code extension. The Toolchain Manager can also be used to open a Terminal with the right settings for building.

You can use VS Code, and the nRF Connect extension to create and write applications, then compile from VS Code (or the Terminal).

The Programmer is then used to load compiled applications onto the Thingy:91 via a USB micro cable, via MCU Boot.

The setup of the Azure certificates in the first half is applicable to all device types. To set up certificates you need:
* PowerShell
* Azure CLI
* OpenSSL

The second half of the article demonstrates using the Thingy:91 to connect. To program this sample you will need:
* Nordic Thingy:91
* LPWAN SIM card (e.g. from Telstra)
* Visual Studio Code

Focus on IPv6
-------------

IPv6, with it's increased address range, is very important for Internet of Things (IoT) projectes, particularly large
scale rollouts.

The Telstra IoT network, including LTE-M and NB-IoT, has full support for IPv6, including IPv6-only connections using
DNS64 (domain name system 6-to-4) and NAT64 (network address translation 6-to-4), which allows devices with an
IPv6-only connection to still connect to IPv4 only servers.

This means, no matter if your servers are IPv6, or still only IPv4, you can use IPv6-only on the Telstra network
and only have to code your application for a single network stack (instead of two).

The Telstra [Wireless Application Development Guidelines](https://www.telstra.com.au/business-enterprise/products/internet-of-things/capabilities) require that all new IOT/M2M devices support IPv6 natively, and IOT/M2M applications and end to end services support IPv6, and that systems are configured either as dual stack or IPv6-only single stack.

Any large-scale deployments are expected to be IPv6-only single stack.

Therefore it is important to evaluate and test any devices for IPv6 capabilities.

The Zephyr operating system, on which the Nordic framework is based, has good support for IPv6, including
IPv6-only networks such as Thread.


Setup of development certificate hierarchy for Azure IoT
--------------------------------------------------------

The Nordic SDK links to some Microsoft guidance for generating certificates, which is partially automated. For a development 
environment we can make a few inmprovements to a fully scripted solution with a root certificate authority plus one 
intermediate authority to issue device certificates.

### Sharing development certificates

To share development certificates between developers you should either share manually, e.g. via USB,
or store them in a secure password manager (e.g. LastPass). This is useful if you have multiple
developers who want to share the same root key stored, but create their own private device keys.

Using multiple levels of intermediate certificates during development is also a good test to ensure
that this architecture will be supported in the live environment

Folders that contain development certificates, including device certificates (particularly the private keys),
should generally be kept outside of source control, or excluded (e.g. `.gitignore`) to prevent leaking of credentials

In some cases it may also be okay to put the development keys under source control to share them, with
them clearly marked for development use only, and that leaking them will not cause a security risk.

### Preparing Azure IoT Hub

See the [Azure Landing](../../azure-landing/README-landing.md) and [Azure IoT](../../azure-iot/README-iot.md) for scripts
to automatically set up Azure Iot Hub.

This will create a unique hub name like `iot-hub001-0xacc5-dev`, derived from your subscription, which is used in
the following scripts.

If you want to test Device Provisioning Service, then the scripts will also create a linked DPS with a name
like `provs-iotcore-0xacc5-dev`. You can retrieve the ID scope of the DPS via:

```powershell
$dps = az iot dps show --name provs-iotcore-0xacc5-dev | ConvertFrom-Json
$dps.properties.idScope
```

You can use your own hub by substituating the name or scope where used.

### OpenSSL environment

When acting as a certificate authority, OpenSSL keeps a minimal database of certificates issued, so needs a
working directory to keep the database and a configuration file for directory settings. We are creating two
certificate authorities, so need two directories, each with a subdirectory for the database and for issued
certificates, plus a directory for device certificates:

```powershell
mkdir dev-certs
cd dev-certs

mkdir rootca
mkdir rootca/certs
mkdir rootca/db
mkdir subca
mkdir subca/certs
mkdir subca/db
mkdir devices
```

Then create a configuration file for the root certificate authority:

```powershell
@'
[ ca ]
default_ca      = ca_default

[ ca_default ]
name            = rootca
home            = $name
database        = $home/db/index.txt
serial          = $home/db/serial.txt
new_certs_dir   = $home/certs
certificate     = $home/$name.pem
private_key     = $home/$name.key
policy          = policy_default
default_md      = sha256
default_days    = 3650
copy_extensions = copy
subjectKeyIdentifier = hash

[ policy_default ]
commonName      = supplied
'@ | Set-Content rootca.cnf
```

And the intermediate certificate authority:

```powershell
@'
[ ca ]
default_ca      = ca_default

[ ca_default ]
name            = subca
home            = $name
database        = $home/db/index.txt
serial          = $home/db/serial.txt
new_certs_dir   = $home/certs
certificate     = $home/$name.pem
private_key     = $home/$name.key
policy          = policy_default
default_md      = sha256
default_days    = 3650
copy_extensions = copy
subjectKeyIdentifier = hash

[ policy_default ]
commonName      = supplied
'@ | Set-Content subca.cnf
```

### Root certificate creation

Set some initial values, generate the root development certificate, and then self-sign it.

```powershell
touch rootca/db/index.txt
openssl rand -hex 16 > rootca/db/serial.txt

openssl req -config rootca.cnf -new -newkey rsa:2048 -nodes `
  -keyout rootca/rootca.key -out rootca/rootca.csr `
  -subj "/CN=Development Root CA" `
  -addext "basicConstraints = critical,CA:true" `
  -addext "keyUsage = critical,keyCertSign,cRLSign"
openssl req -text -in rootca/rootca.csr -noout

openssl ca -config rootca.cnf -selfsign -batch `
  -in rootca/rootca.csr -out rootca/rootca.pem
openssl x509 -in rootca/rootca.pem -text
```

The `-nodes` option means the private key will not be encrypted, and so can be accessed without a password.

By generating and then signing the certificate, it will appear in the root certificate authorities database
and directory of issued certificates (`ls rootca/certs`).

### Intermediate certificate creation

This can be loaded into Azure IoT Hub as the authority for device certificates (or you
can use a longer chain if you want).

```powershell
touch subca/db/index.txt
openssl rand -hex 16 > subca/db/serial.txt

openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout subca/subca.key -out subca/subca.csr `
  -subj "/CN=Development Intermediate CA" `
  -addext "basicConstraints = critical,CA:true" `
  -addext "keyUsage = critical,keyCertSign,cRLSign" `
  -addext "extendedKeyUsage = clientAuth,serverAuth"
openssl req -text -in subca/subca.csr -noout

openssl ca -config rootca.cnf -batch `
  -in subca/subca.csr -out subca/subca.pem
openssl x509 -in subca/subca.pem -text
```

The certificate is generated in the `subca` folder, but is issued by the root authority
(`ls rootca/certs` now has two certificates)

### Load key into Azure and get verification challenge

This is based on the IoT Hub created by the Azure scripts referenced above, with the name based
on the subscription.

You need to load the certificate, then generate a verification challenge (to then sign as proof of
possession of the corresponding private key).

```powershell
az login
az account set --subscription '<your subscription ID>'
$VerbosePreference = 'Continue'

$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
$certName = "cert-0x$((az account show --query id --output tsv).Substring(0,4))-subca"

$cert = az iot hub certificate create --hub-name $iotName --name $certName --path subca/subca.pem | ConvertFrom-Json
$challenge = az iot hub certificate generate-verification-code --hub-name $iotName --name $certName --etag $cert.etag | ConvertFrom-Json
```

### Sign the challenge response and complete verification

Generate a certificate with the challenge as the Common Name and sign it with the intermediate certificate
private key (which the server can verify against the public key):

```powershell
openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout subca/proof.key -out subca/proof.csr `
  -subj "/CN=$($challenge.properties.verificationCode)"
openssl req -text -in subca/proof.csr -noout

openssl ca -config subca.cnf -batch `
  -in subca/proof.csr -out subca/proof.pem
openssl x509 -in subca/proof.pem -text
```

This proof response will be the first certificate issued by the intermediate authority (`ls subca/certs`).

Then upload the result to complete verification:

```powershell
$result = az iot hub certificate verify --hub-name $iotName --name $certName --etag $challenge.etag --path subca/proof.pem | ConvertFrom-Json
$result.properties.isVerified
```

### Cleanup certificate authorities

You can get a list of existing certificates and then delete when no longer needed.

```powershell
$certs = az iot hub certificate list --hub-name $iotName | ConvertFrom-Json
$certs.value | ft name, etag, @{n='isVerified';e={$_.properties.isVerified}}
az iot hub certificate delete --hub-name $iotName --name $certs.value[0].name --etag $certs.value[0].etag
```

Generate device certificates
----------------------------

You will need the device ID that will be used when connecting to Azure. We will use the device
IMEI number, which is printed on a sticker inside the Thingy:91, e.g. "350457791791735879".

This needs to be manually set in the sample, but is the auto-generated default value used in
the Nordic asset tracking application sample. You can also override with your own ID if needed.

Note if you are using DPS it uses a registration ID, which must match the common name on the device certificate. This is a
separate value from the device ID, but will often be the same (e.g. if they must both match the same device certificate).

### Create device in Azure

With the device ID (the IMEI) you can create a device identity in Azure IoT Hub, with the authentication method
of `x509_ca` which indicates the device will use a certificate signed by the authority that we have uploaded.

```powershell
$deviceId = "350457791791735879"
az iot hub device-identity create --hub-name $iotName --device-id $deviceid --am x509_ca
```

If using DPS then the device will automatically be provisioned to the hub as needed.

### Generate device certificate

We then use the device ID to generate the private key for the device, a signing request, and then 
sign with the intermediate key to create a device certificate.

```powershell
openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout "devices/$($deviceId).key" -out "devices/$($deviceId).csr" `
  -subj "/CN=$deviceId" `
  -addext "basicConstraints = critical,CA:false" `
  -addext "keyUsage = critical,digitalSignature" `
  -addext "extendedKeyUsage = clientAuth"
openssl req -text -in "devices/$($deviceId).csr" -noout

openssl ca -config subca.cnf -batch `
  -in "devices/$($deviceId).csr" -out "devices/$($deviceId).pem"
openssl x509 -in "devices/$($deviceId).pem" -text
```

The certificate and private key will be in the devices folder (`ls devices`), and a copy of the issued certificate
will also be in the intermediate authority database (`ls subca/certs`).

The device certificate (and device private key) need to be placed in the device, so the device can
use the device private key to sign challenges, providing the signed response plus the device
certificate.

Azure IoT Hub can then verify the signature against the device certificate, and validate the
device certificate (which is signed by the intermediate certificate) against the 
uploaded intermediate certificate, providing the chain of trust to the device. 

### Cleanup devices

You can list known devices:

```powershell
$devices = az iot hub device-identity list --hub-name $iotName | ConvertFrom-Json
$devices | ft deviceId, authenticationType, status, connectionState, lastActivityTime
```

And remove a device:

```powershell
az iot hub device-identity delete --hub-name $iotName --device-id $deviceid
```

Load certificates to the Thingy:91
----------------------------------

To provision the certificates to the device, we use a combination of the nRF9160: AT Client sample firmware and the
LTE Link Monitor tool, as detailed in https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.3.0/nrf/libraries/networking/azure_iot_hub.html.

You can use the sample as is, so you can just build from the SDK.

You can use the Toolchain Manager to open up a Terminal (in the dropdown next to the SDK version you are using),
change to the directory where your example is, and build:

```powershell
cd nrf/samples/nrf9160/at_client
west build -p -c -b thingy91_nrf9160_ns
```

<pic - build>

### Preparing the Thingy:91

To insert the nano SIM you will need to take the orange rubber cover off the Thingy:91. During development
you will want to keep the cover off so that you can easily access the on/off switch.

<pic - device>

You may also want to update the base firmware on the device -- see the Nordic product site for details.

### Programming

With the hex file build and the SIM card ready, you need to put the device into programming mode.

To do this, using the nRF Connect Desktop to launch the Programmer, connect the micro-USB cable, 
then hold down the button in the middle while turning the Thingy:91 on. Holding the button will
start the device in programming mode.

In the Programmer, select the device from the drop down in the top left, clear any existing files,
and then add the `build/zephyr/app_signed.hex` file that you just built. Make sure that MCUboot
is enabled, which is the component that allows updating over the USB.

Click Write and confirm, and then wait for the image to be uploaded.

### Loading certificates

Restart the device (without the button), and connect to it with the LTE Link Monitor tool, then go to the Certificate Manager tab.

In certificate manager, set:

* Security tag: Enter "10". This is the default in the sample application, or you can choose your own slot to use.
* CA certificate: Open the `azure-certs/BaltimoreCyberTrustRoot.crt.pem` file (or download from Microsoft), and paste the contents in as the CA Certificate.
* Client certificate: Open the device client certificate file (PEM), e.g. `dev-certs/devices/350457791791735879.pem` and paste the contents between (and including) the BEGIN and END lines into Client certifcate.
* Private key: Open the device private key file (KEY), e.g. `dev-certs/devices/350457791791735879.key`, and paste into the Private key.

Make sure you have definitely updated the Security tag and then click Update certificates.

<pic - certificates>

The log will report when it is complete, and you will also see the AT commands that were sent in the Terminal window. You can also use `AT%CMNG=1` to list the certificates in use.

Note that Microsoft is in the process of switching over their certificates, so you can also load the 
`DigiCertGlobalG2TLSRSASHA2562020CA1.crt.pem` file and configure it as a secondary certificate if you
are planning long term use of the device.

Once the certificates are loaded you can turn the device off.


Run the Nordic Azure IoT sample
-------------------------------

You can create your own copy of the sample application (so you can modify it) in VS Code. Use the Toolchain Manager
to open VS Code with the correct SDK and extension.

The NRF Connect extension can then be used to 'Create a new application', select a Freestanding
application and your SDK and Toolchain (e.g. 2.3.0). Set a location to create the application, and use the
template `nrf/samples/nrf9160/azure_iot_hub`. You can use your own name if you want (I used `thingy91_nbiot_azure`).

The Nordic SDK is based on [Zephyr OS](https://github.com/zephyrproject-rtos/zephyr), so you will also see a bunch of
generic Zephyr samples, as well as the Nordic (nrf) specific ones.

Note that when you create the sample, it will initialise the folder as new git repository. If you don't want
this, e.g. the folder is part of a larger repository, you can simply delete the .git folder, and then 
check in the initial code to your source control.

### Configuring the sample

Before building you need to configure the `prj.conf` file with your hub and IoT Hub host name.

```ini
CONFIG_AZURE_IOT_HUB_DEVICE_ID="350457791791735879"
CONFIG_AZURE_IOT_HUB_HOSTNAME="iot-hub001-0xacc5-dev.azure-devices.net"
```

A full example would store these values in configuration (or auto-generate), so that the a single
firmware binary can be deployed across multiple devices.

### Building the sample

With the SDK terminal (opened from Toolchain Manager), change to the directory where you created the sample app,
and build.

```powershell
cd ~/Code/iot-demo-build/nordic/thingy91_nbiot_azure
west build -p -c -b thingy91_nrf9160_ns
```

Then use the Programmer to write it to the Thingy:91 (hold down the button while turning on, clear and select the new file `build/zephyr/app_signed.hex`, and Write).

### Running the sample

To view the debug output stream from the device you can listen to the micro-USB serial port. One
easy way to do this is the nRF Terminal tab in VS Code. You can find this terminal in  
View > Command Palette (and then search for it), or via `CTRL+J` and then selecting the tab.

Click the start button in the terminal (top right) and connect to the serial port
(usually `/dev/ttyACM0` on Linux).

Turn the Thingy:91 off (the terminal will report Disconnected) and then back on again
(without pressing the button) and you will see the device run the Azure IoT Hub sample.

<pic - terminal>


Running in IPv6-only network
----------------------------

If we turn on debugging for the `mqtt_helper` then we will see that the device is connecting
to an IPv4 address -- not surprising as the packet data network connection defaults to dual
stack and Azure IoT Hub currently only has an IPv4 address:

```powershell
sly@sigil:~/Code/iot-demo-build/nordic/thingy91_nbiot_azure$ host iot-hub001-0xacc5-dev.azure-devices.net 8.8.8.8
Aliases: 
iot-hub001-0xacc5-dev.azure-devices.net is an alias for gateway-prod-gw-australiaeast-1-tls10.australiaeast.cloudapp.azure.com.
gateway-prod-gw-australiaeast-1-tls10.australiaeast.cloudapp.azure.com has address 13.70.74.195
```

In dual stack mode, Telstra provides private IPv4 addresses to devices and runs network address 
translation (NAT44) to allow them to connect from that private network to public IPv4 addresses.

### Updating the sample for IPv6

Currently the sample waits for LTE packet data network (PDN) connection, and then tries to connect to Azure.
However IPv6 prefixes are sent in a separate notification after the initial network activation.

This means that if you run the code with IPv6 only it will fail, as it tries to resolve the
address before IPv6 is fully configured.

For this sample we can just add a short delay, in `src/main.c`, after the connection and before initialising Azure:

```c
  ...
	modem_configure();
	k_sem_take(&network_connected_sem, K_FOREVER);
	LOG_INF("Connected to LTE network");
#endif

  /* Wait for IPv6. TODO: Listen for PDN events, with timeout */
  k_msleep(1000);
  ...
```

In a full build we would listen to PDN events and wait for IPv6, with a short timeout (in case the network
does not have IPv6).

### Configuring for IPv6-only

We can easily configure the packet data network for IPv6 only operation in `prj.conf`:

```ini
CONFIG_PDN=y
CONFIG_PDN_DEFAULTS_OVERRIDE=y
CONFIG_PDN_DEFAULT_APN="telstra.iot"
CONFIG_PDN_DEFAULT_FAM_IPV6=y
```

We can also enable debug logging for `mqtt_helper`, the component that connects to Azure, to see the IP address resolution.

```ini
CONFIG_MQTT_HELPER_LOG_LEVEL_DBG=y
```

### Example connection to Azure IoT Hub from IPv6-only network

Rebuild the application and deploy to the device, connect to NRF Terminal and turn the device back on.

The additional logging will show the IPv6 address resolution, and then the successful connection
to Azure IoT Hub:

<pic - logs>

The relevant log lines show the lookup of `iot-hub001-0xacc5-dev.azure-devices.net` and the resolved
IPv6 address of `2001:8004:11d0:4e2a::d46:4ac3`.

```
[00:00:04.396,423] <dbg> mqtt_helper: broker_init: Resolving IP address for iot-hub001-0xacc5-dev.azure-devices.net
[00:00:04.595,703] <dbg> mqtt_helper: broker_init: IPv6 Address found 2001:8004:11d0:4e2a::d46:4ac3 (AF_INET6)
...
[00:00:08.191,101] <dbg> mqtt_helper: mqtt_evt_handler: MQTT mqtt_client connected
```

This is a NAT64 address, although it is using a custom prefix of `2001:8004:11d0:4e2a::` instead of the
standard `64:ff9b::`.

The suffix `d46:4ac3` is the same for standard DNS64 and contains the encoded IPv4 address: 0x0d = 13, 0x46 = 70, 0x4a = 74, 0xc3 = 195. 

NOTE: While the underlying Zephyr OS has good support for IPv6, support in the Nordic `mqtt_helper` is relatively new
and not available in v2.3.0. To run the sample above, I had to use the v-next development version.




