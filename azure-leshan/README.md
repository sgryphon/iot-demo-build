Leshan server on Azure
======================

Server is running dual-stack, as Azure needs IPv4 to access repositories for package install.

The server uses Caddy to automatically provision a TLS certificate for the server host address, with HTTPS traffic forwarded to the Leshan server web UI. Port 80 is also open, so that Caddy can use Let's Encrypt to automatically get a certificate.

Caddy also applies basicauth to the website, requiring a username and password. This is done over HTTPS, so the password is secure.

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

After deployment, the fully qualified domain name (fqdns) is shown, and can be used to access the web interface using HTTPS, e.g.

```
https://lwm2m-0xacc5-dev.australiaeast.cloudapp.azure.com/
```

You will be prompted with to enter a username ('iotadmin') and the web password you specified when running the script.


### SSH access

You can also SSH into the server, to check the application:

```
ssh iotadmin@lwm2m-0xacc5-dev.australiaeast.cloudapp.azure.com
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


