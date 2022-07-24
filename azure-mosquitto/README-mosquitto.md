Mosquitto server on Azure
=========================

Server is running dual-stack, as Azure needs IPv4 to access repositories for package install.

The server uses Certbot to acquire an SSL certificate from LetsEncrypt (with automatic renewal), and then configures Mosquitto to
use that certificate. Mosquitto also has anonymous access turned off, so you must use the supplied password.

The server is configured to automatically shutdown at 19:00+10 (Brisbane time) each night, to save costs if you are running it
in a limited developer subscription. You can adjust this for your timezone.

Requirements:
* PowerShell
* Azure CLI

To deploy, login to Azure CLI, and then run the deployment script. Setting a password is required, with other values set to
reasonable defaults (for Australia, where I am based; you may want to change location).

```powershell
az login
$VerbosePreference = 'Continue'
./deploy-mosquitto.ps1 -MqttPassword YourSecretPassword
```

The public adddresses of the machine are given a unique name, using the subscription prefix by default (but you can use a different OrgId if you want).

After deployment, the fully qualified domain name (fqdns) is shown, and can be used to access the MQTT server using the mosquitto client.

```shell
mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
```

To access the server use the username 'mqttser' and the password you specified when running the script.


### SSH access

You can also SSH into the server, to check the application (the script automatically assigns your local SSH key with access):

```
ssh iotadmin@mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
```

You can then check the status with:

```
sudo systemctl status mosquitto
```

### Stop and start

There are scripts to stop (to save money) and restart (e.g. each day after the automatic shutdown) the server.

```powershell
./stop-mosquitto.ps1
./start-mosquitto.ps1
```

After restarting you need to log on and run the Leshan server (`java -jar ~/leshan-server/leshan-server-demo.jar`)

### Cleanup

When you are finished, you can remove the resource group, which removes the server, network, IP addresses, and other Azure resources
that were deployed.

```powershell
./remove-mosquitto.ps1
```


Testing the Mosquitto server
----------------------------

You can the Mosquitto tools to test the server, https://mosquitto.org/

Or use an online utility like Paho https://www.eclipse.org/paho/index.php?page=clients/js/utility/index.php.

Install if needed, e.g. on Linux:

```
snap install mosquitto-clients
```

### Running the test clients

First, in one terminal, subscribe:

```
mosquitto_sub -h mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com -t test -p 8883 -u mqttuser -P YourSecretPassword
```

Then use another terminal to publish a message:

```
mosquitto_pub -h mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com -t test -m "hello world" -p 8883 -u mqttuser -P YourSecretPassword
```


Troubleshooting
---------------

Connect (SSH) to the server and check the cloud init logs:

```
more /var/log/cloud-init-output.log
```

Also see: https://docs.microsoft.com/en-us/azure/virtual-machines/linux/cloud-init-troubleshooting


References
----------

* https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-18-04-quickstart
* https://github.com/eclipse/mosquitto/blob/master/README-letsencrypt.md


Todo
----

* Have a look at Eclipse Streamsheets (could install on Mqtt server): https://github.com/eclipse/streamsheets


