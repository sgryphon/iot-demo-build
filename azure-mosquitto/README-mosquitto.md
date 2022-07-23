Mosquitto server on Azure
=========================

Server is running dual-stack, as Azure needs IPv4 to access repositories for package install.

The server

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
./deploy-server.ps1 -WebPassword YourSecretPassword
```

The public adddresses of the machine are given a unique name, using the subscription prefix by default (but you can use a different OrgId if you want).

After deployment, the fully qualified domain name (fqdns) is shown, and can be used to access the MQTT server using the mosquitto client.

```shell
mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
```

You will be prompted with to enter a username ('iotadmin') and the web password you specified when running the script.


### SSH access

You can also SSH into the server, to check the application:

```
ssh iotadmin@mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
```


### Stop and start

There are script to stop (to save money) and restart (e.g. each day after the automatic shutdown) the server.

```powershell
./stop-server.ps1
./start-server.ps1
```

After restarting you need to log on and run the Leshan server (`java -jar ~/leshan-server/leshan-server-demo.jar`)

### Cleanup

When you are finished, you can remove the resource group, which removes the server, network, IP addresses, and other Azure resources
that were deployed.

```powershell
./remove-server.ps1
```


Testing the Mosquitto Server
----------------------------

You can use the Leshan demo client to test.

Install Java Runtime Environment if needed:

```
sudo apt install default-jre
```

Download the test client to a working folder:

```
cd ../temp
wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-client-demo.jar
```

### Running the client

Run the demo client, passing in the address of the Azure Leshan server.

```
java -jar ./leshan-client-demo.jar -u lwm2m-0xacc5-dev.australiaeast.cloudapp.azure.com
```

References
----------

* https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-18-04-quickstart
* https://github.com/eclipse/mosquitto/blob/master/README-letsencrypt.md


