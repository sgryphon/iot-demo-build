Matter (& Thread)
=================


Python Matter Server control script
-----------------------------------

[scripts/Send-MatterCommand.ps1](scripts/Send-MatterCommand.ps1)

This PowerShell script can be used to send commands to a [Python Matter Server](https://github.com/home-assistant-libs/python-matter-server)

It opens up a websocket connection, grabs the initial details, sends the command, waits for the response, then closes the websocket connection.

Several examples of usage are given in the script doc comments.

If you are using Home Assistant, you need to have your Matter Server exposed to access it.

If running as a HAOS Add On, then see the documentation for the Matter Server, and configure the port to expose your Matter Server on, e.g. the default 5580.

Example
-------

An important feature of Matter is Binding, allowing a control (e.g. on/off switch) to directly control an actuator (e.g. on/off light).

To configure this you need to set up:
* Access Control List (ACL) permissions for the switch to access the light.
* Binding from the switch on/off client to the light on/off server.

The following scripts can be used to configure the needed settings, which can be checked by reading the attributes.

However, testing with a Zemismart switch + Nanoleaf light, even though the ACL and binding can be configured, it still does not work as expected.

```powershell
$switchNode = 36
$switchEndpoint = 1
$lightNode = 30
$lightEndpoint = 1

$aclAttribute = "0/31/0"
$bindingAttribute = "$switchEndpoint/30/0"
$onOffCluster = 0x0006

# View the ACL of the light
$aclResponse = ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = $lightNode; attribute_path = $aclAttribute } -Verbose
$acl = $aclResponse.result."0/31/0"
$acl | ConvertTo-Json -Depth 10

# Update the ACL by adding a new entry
# WARNING: Make sure you keep the existing Admin ACL, otherwise you will lose access
$newAcl = $acl + @{ "1" = 3; "2" = 2; "3" = @( $switchNode ); "4" = @( @{ "1" = $lightEndpoint } ) }
$aclCommand = @{ node_id = $lightNode; attribute_path = $aclAttribute; value = $newAcl }
$aclCommand | ConvertTo-Json -Depth 10
./Send-MatterCommand.ps1 -Command write_attribute -CommandArgs $aclCommand -Verbose | ConvertTo-Json -Depth 10

# NOTE: You may get a status 135 (0x87 CONSTRAINT_ERROR) response, but it seems to work anyway
./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = $lightNode; attribute_path = $aclAttribute } -Verbose | ConvertTo-Json -Depth 10

# View the Binding of the switch (should be empty [])
./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = $switchNode; attribute_path = $bindingAttribute } -Verbose | ConvertTo-Json -Depth 10

# Add binding from the switch to the light
$binding = @{ node = $lightNode; endpoint = $lightEndpoint; cluster = $onOffCluster }
$bindingCommand = @{ node_id = $switchNode; attribute_path = $bindingAttribute; value = @( $binding ) }
./Send-MatterCommand.ps1 -Command write_attribute -CommandArgs $bindingCommand -Verbose | ConvertTo-Json -Depth 10

# Check it worked
./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = $switchNode; attribute_path = $bindingAttribute } -Verbose | ConvertTo-Json -Depth 10
```

To remove the entries:

```powershell
# Clear the binding command
$bindingCommand = @{ node_id = $switchNode; attribute_path = $bindingAttribute; value = @() }
./Send-MatterCommand.ps1 -Command write_attribute -CommandArgs $bindingCommand -Verbose | ConvertTo-Json -Depth 10

# Remove the added Operate (3) ACL
$aclResponse = ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = $lightNode; attribute_path = $aclAttribute } -Verbose
$acl = $aclResponse.result."0/31/0"
$acl | ConvertTo-Json -Depth 10

$newAcl = $acl | Where-Object { $_."1" -ne 3 }
$aclCommand = @{ node_id = $lightNode; attribute_path = $aclAttribute; value = $newAcl }
$aclCommand | ConvertTo-Json -Depth 10
./Send-MatterCommand.ps1 -Command write_attribute -CommandArgs $aclCommand -Verbose | ConvertTo-Json -Depth 10
```
