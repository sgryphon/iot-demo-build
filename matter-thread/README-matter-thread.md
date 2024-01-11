Matter (& Thread)
=================


Python Matter Server control script
-----------------------------------

[scripts/Send-MatterCommand.ps1]

This PowerShell script can be used to send commands to a [Python Matter Server](https://github.com/home-assistant-libs/python-matter-server)

It opens up a websocket connection, grabs the initial details, sends the command, waits for the response, then closes the websocket connection.

Several examples of usage are given in the script doc comments.

