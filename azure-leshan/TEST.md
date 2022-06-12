Testing the Leshan Server
=========================

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

Running the client
------------------

Run the demo client, passing in the address of the Azure Leshan server.

```
java -jar ./leshan-client-demo.jar -u lwm2m-0xacc5-dev.australiaeast.cloudapp.azure.com
```
