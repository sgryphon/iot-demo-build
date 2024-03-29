Thingy:91 with Azure IoT Hub
============================

The Nordic NRF SDK includes a sample for connecting an NRF9160 based device (such as the Thingy:91)
to Azure IoT Hub, including support for Device Provisioning Service (DPS).

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/azure_iot_hub/README.html

This example uses the Nordic Azure IoT Hub library:

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/azure_iot_hub.html

The library is based on X.509 CA Signed security for client devices, with devices having a certificate and
key signed by a certificate authority registered in IoT Hub (or Device Provisioning Service).

Setup of development certificates for Azure IoT
-----------------------------------------------

This is a combination of the following pages, for fully scripted creation of a development certificate:

* https://learn.microsoft.com/en-us/azure/iot-hub/tutorial-x509-openssl
* https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-x509ca-overview
* https://www.digicert.com/kb/ssl-support/openssl-quick-reference-guide.htm

It uses a combination of PowerShell, Azure CLI, and OpenSSL.

The generated certificates are created in a subfolder `certs` which is excluded from git, to avoid any
secret keys being checked into source control.

To share development certificates between developers you should either share manually, e.g. via USB,
or store them in a secure password manager (e.g. LastPass). This is useful if you have multiple
developers who want to share the same root key stored, but create their own private device keys.

It may also be okay to put the private development keys under source control to share them, with
them clearly marked for development use only.

You also need to consider security when loading keys onto devices; compiling them into the firmware
is not safe and should only be used for internal development. For security you can use the LTE Link
Monitor tool to load the certificates directly onto the modem.

For more details see: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/azure_iot_hub.html#provisioning-of-the-certificates

### Preparing Azure IoT Hub

See the [Azure Landing](../../azure-landing/README-landing.md) and [Azure IoT](../../azure-iot/README-iot.md) for scripts
to automatically set up Azure Iot Hub.

This will create a unique hub name like `iot-hub001-0xacc5-dev`, derived from your subscription, which is used in
the following scripts.

### Root certificate creation

When acting as a certificate authority, OpenSSL keeps a minimal database of certificates issued, so needs a
working directory (the `certs` folder) that is referenced from a configuration file. Note that the paths are
relative to where you are running the command, so run them in the project folder.

```powershell
mkdir dev-certs
mkdir dev-certs/rootca
mkdir dev-certs/rootca/certs
mkdir dev-certs/rootca/db
touch dev-certs/rootca/db/index.txt

openssl rand -hex 16 > dev-certs/rootca/db/serial.txt
openssl req -config rootca.cnf -new -newkey rsa:2048 -nodes `
  -keyout dev-certs/rootca/rootca.key -out dev-certs/rootca/rootca.csr `
  -subj "/CN=Development Root CA" `
  -addext "basicConstraints = critical,CA:true" `
  -addext "keyUsage = critical,keyCertSign,cRLSign"
openssl req -text -in dev-certs/rootca/rootca.csr -noout

openssl ca -config rootca.cnf -selfsign -batch `
  -in dev-certs/rootca/rootca.csr -out dev-certs/rootca/rootca.pem
openssl x509 -in dev-certs/rootca/rootca.pem -text
```

The `-nodes` option means the private key will not be encrypted,
and so can be accessed without a password.

### Intermediate certificate creation

This can be loaded into Azure IoT Hub as the authority for device certificates (or you
can use a longer chain if you want).

```powershell
mkdir dev-certs/subca
mkdir dev-certs/subca/certs
mkdir dev-certs/subca/db
touch dev-certs/subca/db/index.txt

openssl rand -hex 16 > dev-certs/subca/db/serial.txt
openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout dev-certs/subca/subca.key -out dev-certs/subca/subca.csr `
  -subj "/CN=Development Intermediate CA" `
  -addext "basicConstraints = critical,CA:true" `
  -addext "keyUsage = critical,keyCertSign,cRLSign" `
  -addext "extendedKeyUsage = clientAuth,serverAuth"
openssl req -text -in dev-certs/subca/subca.csr -noout

openssl ca -config rootca.cnf -batch `
  -in dev-certs/subca/subca.csr -out dev-certs/subca/subca.pem
openssl x509 -in dev-certs/subca/subca.pem -text
```

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

$cert = az iot hub certificate create --hub-name $iotName --name $certName --path dev-certs/subca/subca.pem | ConvertFrom-Json
$challenge = az iot hub certificate generate-verification-code --hub-name $iotName --name $certName --etag $cert.etag | ConvertFrom-Json
```

### Sign the challenge response and complete verification

Generate a certificate with the challenge as the Common Name and sign it with the intermediate certificate
private key (which the server can verify against the public key):

```powershell
openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout dev-certs/subca/proof.key -out dev-certs/subca/proof.csr `
  -subj "/CN=$($challenge.properties.verificationCode)"
openssl req -text -in dev-certs/subca/proof.csr -noout

openssl ca -config subca.cnf -batch `
  -in dev-certs/subca/proof.csr -out dev-certs/subca/proof.pem
openssl x509 -in dev-certs/subca/proof.pem -text
```

Then upload the result to complete verification:

```powershell
$result = az iot hub certificate verify --hub-name $iotName --name $certName --etag $challenge.etag --path dev-certs/subca/proof.pem | ConvertFrom-Json
$result.properties.isVerified
```

### Cleanup

You can get a list of existing certificates and then delete when no longer needed.

```powershell
$certs = az iot hub certificate list --hub-name $iotName | ConvertFrom-Json
$certs.value | ft name, etag, @{n='isVerified';e={$_.properties.isVerified}}
az iot hub certificate delete --hub-name $iotName --name $certs.value[0].name --etag $certs.value[0].etag
```

### Web servers

You can also use the certificate chain to sign web server certificates.

NOTE: For web servers use `-addext "extendedKeyUsage = serverAuth"`

Generate client device certificate
----------------------------------

You will need the device ID that will be registered. This can be any arbitrary value, but has to match what
the device will send and is usually based on some characteristic such as a hardware ID, MAC address, IMEI
number, or ICCID.

Generate the private key for the device, a signing request, and then sign with the intermediate key to
create a device certificate.

```powershell
mkdir dev-certs/devices
$deviceId = "imei-350457791791735879"
openssl req -config subca.cnf -newkey rsa:2048 -nodes `
  -keyout "dev-certs/devices/$($deviceId).key" -out "dev-certs/devices/$($deviceId).csr" `
  -subj "/CN=$deviceId" `
  -addext "basicConstraints = critical,CA:false" `
  -addext "keyUsage = critical,digitalSignature" `
  -addext "extendedKeyUsage = clientAuth"
openssl req -text -in "dev-certs/devices/$($deviceId).csr" -noout

openssl ca -config subca.cnf -batch `
  -in "dev-certs/devices/$($deviceId).csr" -out "dev-certs/devices/$($deviceId).pem"
openssl x509 -in "dev-certs/devices/$($deviceId).pem" -text
```

The device certificate (and device private key) need to be placed in the device, so the device can
use the device private key to sign challenges, providing the signed response plus the device
certificate.

Azure IoT Hub can then verify the signature against the device certificate, and validate the
device certificate (which is signed by the intermediate certificate) against the 
uploaded intermediate certificate, providing the chain of trust to the device. 


Alternative certificates
------------------------

The certificates need to be converted to a format suitable for including in the application source.

```powershell
mkdir dev-certs
mkdir dev-certs/devices

$deviceId = "350457791791735879"
$source = "/home/sly/Documents/Temp/dev-certs/devices"
$destination = "dev-certs/devices"

$lines = Get-Content (Join-Path $source "$($deviceId).pem")
$foundStart = $false
$wrapped = $lines | ForEach-Object { if ($_ -match '-----BEGIN') { $foundStart = $true }; if ($foundStart) { "`"$($_)\n`"" }}
$wrapped | Set-Content (Join-Path $destination "$($deviceId).pem.h")

$lines = Get-Content (Join-Path $source "$($deviceId).key")
$foundStart = $false
$wrapped = $lines | ForEach-Object { if ($_ -match '-----BEGIN') { $foundStart = $true }; if ($foundStart) { "`"$($_)\n`"" }}
$wrapped | Set-Content (Join-Path $destination "$($deviceId).key.h")
```


Creating an application
-----------------------

Using the nRF Connect extension for Visual Studio Code, create a new application.

Select the version of the SDK (currently v2.3.0) and the `nrf/samples/nrf9160/azure_iot_hub` template.

Select the location and application name.

Note that the application will be created with it's own git repository; to include it in a higher level repository, simply delete the `.git` (hidden) folder and commit the initial code.

### Register the device

Register the device with the authentication methoda "x509_ca".

```powershell
az iot hub device-identity create --hub-name $iotName --device-id $deviceid --am x509_ca
```


### Provisioning certificates via code

You can follow the process to manually provision the certificates directly to the modem (keeping them secure),
outlined in the application sample.

To provision via code, during development, you need to make a number of changes. Provisioning via code is supported
in mqtt_helper, but only for provisioning via `tls_credential_add()`. The code to provision via `modem_key_mgmt_write()` 
has been adapted from several other samples.

Note that including the certificates in the source code during development means that those specific certificates,
including the private key, are compiled into the firmware image. Not only is this a security risk, but means the firmware
image will only work on one device.

For broader rollout, the device ID needs to be generated from information stored on the device, such as the IMEI, and
the certificates, or other secrets, provisioned directly to the device storage (e.g. following the process from Nordic).
This allows a single firmware image to be used across multiple devices.


#### Converting certificates to C source files

The certificates need to be converted to a format suitable for including in the application source.

```powershell
$lines = Get-Content "dev-certs/devices/$($deviceId).pem"
$foundStart = $false
$wrapped = $lines | ForEach-Object { if ($_ -match '-----BEGIN') { $foundStart = $true }; if ($foundStart) { "`"$($_)\n`"" }}
$wrapped | Set-Content "dev-certs/devices/$($deviceId).pem.h"

$lines = Get-Content "dev-certs/devices/$($deviceId).key"
$foundStart = $false
$wrapped = $lines | ForEach-Object { if ($_ -match '-----BEGIN') { $foundStart = $true }; if ($foundStart) { "`"$($_)\n`"" }}
$wrapped | Set-Content "dev-certs/devices/$($deviceId).key.h"
```

You also need to download the Azure IoT server certificates and convert them for inclusion. Currently this is Baltimore Cyber,
but they are transitioning to DigiCert, so include both. These are public certificates, so have beend downloaded, converted,
and included in the `certs` folder.

#### Updating the sample code to include the certificates

* Prepare a header to include the device-specific certificate files (plus the server files), `credentials_provision\certs-350457791791735879.h`

```c
static const unsigned char ca_certificate[] = {
#include "../certs/BaltimoreCyberTrustRoot.crt.pem.h"
};

static const unsigned char private_key[] = {
#include "../../dev-certs/devices/350457791791735879.key.h"
};

static const unsigned char device_certificate[] = {
#include "../../dev-certs/devices/350457791791735879.pem.h"
};

static const unsigned char ca_certificate_2[] = {
#include "../certs/DigiCertGlobalG2TLSRSASHA2562020CA1.crt.pem.h"
};
```

* Add code to provision via `modem_key_mgmt_write()`, in `credentials_provision\credentials_provision.c` and `credentials_provision\credentials_provision.h`
  * The `credentials_provision.c` file references the setting `CONFIG_AZURE_IOT_HUB_SAMPLE_MODEM_CERTIFICATES` to make it easy to change devices.
* Add the configuration setting to KConfig

```
config AZURE_IOT_HUB_SAMPLE_MODEM_CERTIFICATES
	string "Modem certificates"
	help
	  Include file that contains certificates to provision to the modem.
```

* Modify `main.c` to include the header and call the function before initialising the modem, but only if `CONFIG_MODEM_KEY_MGMT` is set.

```c
//#if defined(CONFIG_MODEM_KEY_MGMT)
#include "credentials_provision.h"
//#endif /* CONFIG_MODEM_KEY_MGMT */

...

#if CONFIG_MODEM_KEY_MGMT
	err = credentials_provision();
	if (err) {
		LOG_ERR("credentials_provision, error: %d", err);
		return;
	}
#endif /* CONFIG_MODEM_KEY_MGMT */
```

* Modify `CMakeLists.txt` to include the header and source files, when `CONFIG_MODEM_KEY_MGMT` is set.

```
if (CONFIG_MODEM_KEY_MGMT) 
  target_sources(app PRIVATE credentials_provision/credentials_provision.c)
endif ()
zephyr_include_directories_ifdef(CONFIG_MODEM_KEY_MGMT credentials_provision)
```

* Create an overlay file for the device `overlay-device-350457791791735879.conf` with the IoT Hub host name, device name, enabling `CONFIG_MODEM_KEY_MGMT`, setting `CONFIG_AZURE_IOT_HUB_SAMPLE_MODEM_CERTIFICATES`, and the secondary tag `CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG`.

To get the IoT Hub host name:

```powershell
$hub = az iot hub show --name $iotName | ConvertFrom-Json
Write-Host "CONFIG_AZURE_IOT_HUB_HOSTNAME=`"$($hub.properties.hostName)`"`nCONFIG_AZURE_IOT_HUB_DEVICE_ID=`"$deviceId`""
```

The overlay will look like this:

```ini
CONFIG_AZURE_IOT_HUB_HOSTNAME="iot-hub001-0xacc5-dev.azure-devices.net"
CONFIG_AZURE_IOT_HUB_DEVICE_ID="350457791791735879"
CONFIG_MODEM_KEY_MGMT=y
CONFIG_AZURE_IOT_HUB_SAMPLE_MODEM_CERTIFICATES="certs-350457791791735879.h"
CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG=11
```

Building and running
--------------------

To build, using the overlay configured above:

```powershell
west build -p -c -b thingy91_nrf9160_ns -- "-DOVERLAY_CONFIG=overlay-device-350457791791735879.conf"
```

Use the nRF Connect Programmer to write the `zephyr/app_signed.hex` file to the device and restart.

As well as monitoring the serial logs from the device you can monitor IoT Hub activity:

```powershell
az iot hub monitor-events -n $iotName --timeout 0

Starting event monitor, use ctrl-c to stop...
{
    "event": {
        "origin": "imei-350457791791735879",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{\"temperature\":25.1,\"timestamp\":49101}"
    }
}
{
    "event": {
        "origin": "imei-350457791791735879",
        "module": "",
        "interface": "",
        "component": "",
        "payload": "{\"temperature\":25.1,\"timestamp\":69111}"
    }
}
```