Mosquitto server on Azure
=========================

Server is running dual-stack, with the primary access via IPv6, however Azure also needs IPv4 for management access.

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
az account set --subscription 'acc54a93-d63e-4462-953e-dbeba4387b12'
$VerbosePreference = 'Continue'
./deploy-mosquitto.ps1 YourSecretPassword
```

The public adddresses of the machine are given a unique name, using the subscription prefix by default (but you can use a different OrgId if you want).

After deployment, the fully qualified domain name (fqdns) is shown, and can be used to access the MQTT server using an MQTT client, with the username 'mqttuser' and the password you specified when running the script.

The script also creates additional users 'mqttservice', 'dev00001', 'dev00002', and 'dev00003', with the password you specified with the suffix '2', '3', '4', and '5', repectively.

Testing the Mosquitto server
----------------------------

You can the Mosquitto tools to test the server, https://mosquitto.org/

Or use an online utility like Paho https://www.eclipse.org/paho/index.php?page=clients/js/utility/index.php.

Install if needed, e.g. on Linux:

```shell
snap install mosquitto-clients
```

### Running the test clients

First, in one terminal, subscribe:

```powershell
$servicePassword = 'YourSecretPassword2'
mosquitto_sub -h mqdev01-0xacc5.australiaeast.cloudapp.azure.com -t 'dt/demo/#' -F '%I %t [%l] %p' -v -p 8883 -u mqttservice -P $servicePassword
```

Then use another terminal to publish a message (note that mqttdevice1 has the password suffix '3'):

```powershell
$device1Password = 'YourSecretPassword3'
mosquitto_pub -h mqdev01-0xacc5.australiaeast.cloudapp.azure.com -t 'dt/demo/1/dev00001/senml' -m '[{\"bn\":\"dev00001_\",\"n\":\"temperature\",\"u\":\"Cel\",\"v\":23.1}]' -p 8883 -u dev00001 -P $device1Password
```

Example output (also showing the log tail on the server):

![Mosquitto test](pics/mosquitto-test.png)


Server management
-----------------

### SSH access

You can also SSH into the server, to check the application (the script automatically assigns your local SSH key with access):

```
ssh iotadmin@mqdev01-0xacc5.australiaeast.cloudapp.azure.com
```

You can then follow the Mosquitto logs with:

```
sudo tail -f /var/log/mosquitto/mosquitto.log
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

### IPv4

If you do not have IPv6 available, then there is also an IPv4 endpoint:

```powershell
$mqttPassword = 'YourSecretPassword'
mosquitto_sub -h mqdev01v4-0xacc5.australiaeast.cloudapp.azure.com -t '#' -F '%I %t [%l] %p' -p 8883 -u mqttuser -P $mqttPassword
```

Security
--------

Mosquitto can validate the username (via the password or other mechanisms), however by default there is also no validation of client ID (i.e. any user can use any client ID), which is mainly used for managing persistent subscriptions. (The dynamic security plugin can limit client ID usage)

Also, by default any authenticated user can send messages to any topic, however this means the subscribers have no way to know who a message is really from, and a single compromised user credentials could have access to the entire MQTT broker.

The main metadata that a recipient receives is just the topic name (and the message itself).

Security principles of least privilege mean that you usually want to restrict access. For example, with an IoT system devices will be publishing messages (such as sensor readings) for themselves, and be subscribing to messages destined for themselves.

To enforce this topic publishing need to be restricted to specific usernames, allowing recipients to be assured who messages are from (i.e. have the username identifier as part of the topic). Having destination username identifiers in the topic can also help with routing.

Many protocols have this type of system, e.g.

* Azure IoT devices publish to `devices/{device-id}/messages/events` and subscribe to `devices/{device-id}/messages/devicebound/#`
* LwM2M clients subscribe and publish to `{tenant/}lwm2m/rd/{endpoint-client-name}`
* OneM2M publishes requests to `/oneM2M/req/{Originator-ID}/{Receiver-ID}/{type}` and subscribe to responses on `/oneM2M/resp/{Originator-ID}/{Receiver-ID}`
* SparkPlug publishes and subscribes to `spBv1.0/{group_id}/{message_type}/{edge_node_id}[/device_id]` based on the message_type, e.g. publish `DDATA` and subscribe to `DCMD`.
* AWS device shadows subscribe and publish to topics like `$aws/things/{thingName}/shadow/update`
* Generic structure for AWS topics for data is `dt/<application>/<context>/<thing-name>/<dt-type>`, and for commands is `cmd/<application>/<context>/<destination-id>/<req-type> ` for requests and `cmd/<application>/<context>/<destination-id>/<res-type>` for responses.

AWS has some general guidelines for creating MQTT topics: <https://docs.aws.amazon.com/whitepapers/latest/designing-mqtt-topics-aws-iot-core/designing-mqtt-topics-aws-iot-core.html>

This include distguishing data topics from command topics and include the thing name (device ID) in any topics for publishing or subscribing. Including the device ID can be useful for routing (e.g. the device only needs to subscribe to limited  topics).

### Mosquitto access control list

The server installation `cloud-init` also sets up topic rules to restrict messages from device usernames to corresponding topics, using for pattern (template) access. These paterns apply for the `dev000..` users:

* Write (publish) `dt/demo/+/%u/#` (for data)
* ReadWrite (publish and subscribe) `cmd/demo/+/%u/#` (for command requests and responses)

In practice, devices will subscribe to specific `<req-type>` paths under their username, and publish on others, and only for specific `<context>` values.

The `mqttservice` user has broader permissions to:

* Read (subscribe) `dt/demo/#` (all data messages)
* ReadWrite (subscribe to) `cmd/demo/#` (for sending command requests and listening to responses)

The `mqttuser` has access permissions to everything (readwrite for `#`) and can be used for troubleshooting

This ensures that when a message is received on a topic with a specific username, we know it is from that username, and that only `mqttservice` can publish to the general `cmd` topic. (Note that compromised device credentials could also publish commands for itself, although not other devices, so you could lock that down further if needed.)

If you try to send to a topic you don't have permission to, it won't work:

```powershell
$device1Password = 'YourSecretPassword3'
mosquitto_pub -h mqdev01-0xacc5.australiaeast.cloudapp.azure.com -t 'dt/demo/1/dev00002' -m 'Dev 1 sending a message on Dev 2 topic does not work' -p 8883 -u dev00001 -P $device1Password
```

Insecure server
---------------

Some devices are not capable of MQTTS (TLS) connections; they only support insecure connections.

To safely deploy such devices, you can use private networks -- and for NB-IoT devices a private APN (Access Point Name) -- to make them secure.

In those case you may want to test with an insecure MQTT server that does allow connection on port 1883.

The script allows you create additional servers (e.g. number 002), and configure them to allow insecure access.

```powershell
az login
$VerbosePreference = 'Continue'
./deploy-mosquitto.ps1 -MqttPassword YourInsecurePassword -ServerNumber 2 -AllowInsecure
```

To test your connection:

```powershell
$mqttPassword = 'YourInsecurePassword'
mosquitto_sub -h mqdev02-0xacc5.australiaeast.cloudapp.azure.com -t '#' -F '%I %t [%l] %p' -p 1883 -u mqttuser -P $mqttPassword
```

And then send:

```powershell
$device1Password = 'YourInsecurePassword3'
mosquitto_pub -h mqdev02-0xacc5.australiaeast.cloudapp.azure.com -t 'dt/demo/test/dev00001' -m 'Insecure' -p 1883 -u dev00001 -P $device1Password
```


Troubleshooting
---------------

Connect (SSH) to the server and check the cloud init logs:

```shell
more /var/log/cloud-init-output.log
```

Also see: https://docs.microsoft.com/en-us/azure/virtual-machines/linux/cloud-init-troubleshooting

Test if Mosquitto is working on the server, e.g

```shell
sudo systemctl status mosquitto
sudo tail /var/log/mosquitto/mosquitto.log
mosquitto_pub -h localhost -t test -m "hello world" -u mqttuser -P YourSecretPassword
```

From a client you can also test if you can connect to the port, and if SSL is working.

```shell
nc -vz mqdev01-0xacc5.australiaeast.cloudapp.azure.com 8883
echo "Q" | openssl s_client -showcerts mqdev01-0xacc5.australiaeast.cloudapp.azure.com:8883
```


References
----------

* https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-18-04-quickstart
* https://github.com/eclipse/mosquitto/blob/master/README-letsencrypt.md


Todo
----

* Have a look at Eclipse Streamsheets (could install on Mqtt server): https://github.com/eclipse/streamsheets
