Linux server on Azure
======================

Server is running dual-stack, as Azure needs IPv4 to access repositories for package install.

The server uses Caddy to automatically provision a TLS certificate for the server host address, with HTTPS traffic forwarded to port 5000. Port 80 is also open, so that Caddy can use Let's Encrypt to automatically get a certificate.

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
./deploy-server.ps1
```

The public adddresses of the machine are given a unique name, using the subscription prefix by default (but you can use a different OrgId if you want).

After deployment, the fully qualified domain name (fqdns) is shown, and can be used to access the web interface using HTTPS (once you have an application running on it), e.g.

```
https://wsdev01-0xacc5.australiaeast.cloudapp.azure.com/
```

Caddy is set up to forward to port 5000, so if you run a .NET web application on that port it will be served up on the configured public address.

### SSH access

You can also SSH into the server, to check the application:

```
ssh iotadmin@wsdev01-0xacc5.australiaeast.cloudapp.azure.com
```


### Stop and start

There are script to stop (to save money) and restart (e.g. each day after the automatic shutdown) the server.

```powershell
./stop-server.ps1
./start-server.ps1
```

### Cleanup

When you are finished, you can remove the resource group, which removes the server, network, IP addresses, and other Azure resources
that were deployed.

```powershell
./remove-server.ps1
```


Notes
-----

pwsh
./start-server.ps1
ssh iotadmin@wsdev01-0xacc5.australiaeast.cloudapp.azure.com
cd /opt/iot-demo-build/telstra-digital-twins/demo-data/tdt-simulator
sudo dotnet run

https://wsdev01-0xacc5.australiaeast.cloudapp.azure.com/OccupancyGenerator?api_key=hV9N1J4vDY46h19r

https://wsdev01v4-0xacc5.australiaeast.cloudapp.azure.com/OccupancyGenerator?api_key=hV9N1J4vDY46h19r


**Troubleshoot: Caddy logs**

journalctl -f -u caddy

sudo systemctl restart caddy

sudo systemctl stop caddy
sudo systemctl start caddy


wget -q -O- localhost:5000/OccupancyGenerator?api_key=hV9N1J4vDY46h19r


Preference in dual stack is IPv4:
wget -q -O- https://v4v6.ipv6-test.com/api/myip.php

But IPv6 only works:
wget -q -O- https://v6.ipv6-test.com/api/myip.php


**References:**

https://stackoverflow.com/questions/60054951/running-multiple-asp-net-core-3-1x-latest-websites-on-port-80-with-kestrel

https://davemateer.com/2020/01/09/Publishing-ASP-NET-Core-3-App-to-Ubuntu
