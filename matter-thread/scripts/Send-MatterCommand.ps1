#!/usr/bin/env pwsh

<#
  .SYNOPSIS
    Send Matter command to Python Matter Server using websockets

  .EXAMPLE
    Node details via get_node command:

    $n14 = ./Send-MatterCommand.ps1 -Command get_node -CommandArgs @{ node_id = 14 } -Verbose
    $n14.result.attributes.'0/40/1', $n4.result.attributes.'0/40/3', $n4.result.attributes.'0/40/15'
    $n14 | ConvertTo-Json -Depth 10

  .EXAMPLE
    Get attribute OnOff.OnOff via read_attribute

    ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = 14; attribute_path = "1/6/0" } -Verbose | ConvertTo-Json -Depth 10

    ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = 12; attribute_path = "1/6/0" } -Verbose | ConvertTo-Json -Depth 10

  .EXAMPLE
    Get attribute OperationCredentials.CurrentFabricIndex via read_attribute; useful to know what the fabric index is within the node.

    ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = 14; attribute_path = "0/62/5" } -Verbose | ConvertTo-Json -Depth 10

  .EXAMPLE
    Send the On and Off commands to an endpoint cluster via device_command:

    $ca = @{ node_id = 12; endpoint_id = 1; cluster_id = 0x0006; command_name = "On"; payload = @{} }
    ./Send-MatterCommand.ps1 -Command device_command -CommandArgs $ca -Verbose | ConvertTo-Json -Depth 10

    $ca = @{ node_id = 12; endpoint_id = 1; cluster_id = 0x0006; command_name = "Off"; payload = @{} }
    ./Send-MatterCommand.ps1 -Command device_command -CommandArgs $ca -Verbose | ConvertTo-Json -Depth 10

  .EXAMPLE

    Get the global scene (group 0, scene 0), used by OnWithRecallGlobalScene

    $ca = @{ node_id = 12; endpoint_id = 1; cluster_id = 0x0005; command_name = "ViewScene"; payload = @{ '0' = 0; '1' = 0 } }
    ./Send-MatterCommand.ps1 -Command device_command -CommandArgs $ca -Verbose | ConvertTo-Json -Depth 10

    # Didn't work for Wiz tunable white light
    $ca = @{ node_id = 12; endpoint_id = 1; cluster_id = 0x0006; command_name = "OnWithRecallGlobalScene"; payload = @{} }
    ./Send-MatterCommand.ps1 -Command device_command -CommandArgs $ca -Verbose | ConvertTo-Json -Depth 10

  .EXAMPLE
    Get attribute Binding.Binding

    ./Send-MatterCommand.ps1 -Command read_attribute -CommandArgs @{ node_id = 14; attribute_path = "1/30/0" } -Verbose | ConvertTo-Json -Depth 10

  .EXAMPLE
    Configure a binding using write_attribute to configure the list of bindings, for a specific fabric index.
    Note this is the fabric index on the node, not the fabric ID; read attribute OperationalCredentials.CurrentFabricIndex 

    # Didn't fully work on Zemismart ZME2 -- the binding was set, but didn't do anything.
    $binding = @{ node = 12; group = $null; endpoint = 1; cluster = 0x0006; fabricIndex = 2 }
    $ca = @{ node_id = 14; attribute_path = "1/30/0"; value = @( $binding ) }
    ./Send-MatterCommand.ps1 -Command write_attribute -CommandArgs $ca -Verbose | ConvertTo-Json -Depth 10


#>
[CmdletBinding()]
param (
  # Command to send, e.g. get_node, read_attribute
  [Alias("c")]
  [Parameter(Mandatory = $true)]
  [string] $Command,
  
  # Command args, if any, as an object, e.g. @{ node_id = 4 }
  [Alias("a")]
  $CommandArgs,

  # Message ID to use, defaults to a random GUID
  [Alias("i")]
  [string] $MessageId = [Guid]::NewGuid().ToString(),

  # The Python Matter Server to connect to, usually "ws://<server-name>:5580/ws"
  [Alias("s")]
  [string] $ServerUri = "ws://pi1.lan:5580/ws",

  # General timeout (for connect, send, close, etc)
  [int] $TimeoutMilliseconds = 5000,

  # Timeout for receiving response
  [int] $ReceiveTimeoutMilliseconds = 60000,

  [int] $ReceiveBufferSizeBytes = 100000
)

Write-Verbose "Message ID: $MessageId"

$message = @{ message_id = $MessageId; command = $Command; args = $CommandArgs }
$messageJson = $message | ConvertTo-Json -Depth 20 -Compress

Write-Verbose "JSON message: $messageJson"

$timeoutStepMilliseconds = 100

# Connect

$timeoutCounter = 0;
Write-Progress -Activity "Sending Command $Command" -Status "Connecting $ServerUri"
Write-Verbose "Connecting"

$webSocket = New-Object System.Net.WebSockets.ClientWebSocket
$ct = New-Object System.Threading.CancellationToken

$connection = $webSocket.ConnectAsync($ServerUri, $ct)

while (-not $connection.IsCompleted)
{
  Start-Sleep -Milliseconds $timeoutStepMilliseconds
  $timeoutCounter += $timeoutStepMilliseconds
  Write-Progress -Activity "Sending Command $Command" -Status "Connecting $ServerUri $timeoutCounter ms"
  if ($timeoutCounter -gt $TimeoutMilliseconds) { throw "Waiting for ConnectAsync timed out" }
}

# Receive connect response:

$buffer = New-Object byte[] $ReceiveBufferSizeBytes
$bufferSegment = New-Object System.ArraySegment[byte] -ArgumentList @(,$buffer)
$receiveConnected = $webSocket.ReceiveAsync([ArraySegment[byte]]$buffer, $ct)

$timeoutCounter = 0;
Write-Progress -Activity "Sending Command $Command" -Status "Connected"
while (-not $receiveConnected.IsCompleted)
{
  Start-Sleep -Milliseconds $timeoutStepMilliseconds
  $timeoutCounter += $timeoutStepMilliseconds
  Write-Progress -Activity "Sending Command $Command" -Status "Connected $timeoutCounter ms"
  if ($timeoutCounter -gt $TimeoutMilliseconds) { throw "Waiting for connection reponse timed out" }
}
Write-Progress -Activity "Sending Command $Command" -Status "Connected"

$connectJson = [System.Text.Encoding]::UTF8.GetString($buffer)
$connectResponse = ConvertFrom-Json $connectJson
Write-Verbose "Fabric $($connectResponse.fabric_id) ($($connectResponse.compressed_fabric_id)), Schema $($connectResponse.schema_version), SDK $($connectResponse.sdk_version)"

# Send message:

$timeoutCounter = 0;
Write-Progress -Activity "Sending Command $Command" -Status "Sending"
Write-Verbose "Sending"

$bytes = [System.Text.Encoding]::UTF8.GetBytes($messageJson)
$bytesSegment = New-Object System.ArraySegment[byte] -ArgumentList @(,$bytes)
$send = $webSocket.SendAsync($bytesSegment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, $ct)

while (-not $send.IsCompleted)
{
  Start-Sleep -Milliseconds $timeoutStepMilliseconds
  $timeoutCounter += $timeoutStepMilliseconds
  Write-Progress -Activity "Sending Command $Command" -Status "Sending $timeoutCounter ms"
  if ($timeoutCounter -gt $TimeoutMilliseconds) { throw "Waiting for SendAsync timed out" }
}
Write-Progress -Activity "Sending Command $Command" -Status "Sent"

# Receive response:

$timeoutCounter = 0;
Write-Progress -Activity "Sending Command $Command" -Status "Receiving response"
Write-Verbose "Receiving response"

$buffer = New-Object byte[] $ReceiveBufferSizeBytes
$bufferSegment = New-Object System.ArraySegment[byte] -ArgumentList @(,$buffer)
$receive = $webSocket.ReceiveAsync([ArraySegment[byte]]$buffer, $ct)

while (-not $receive.IsCompleted)
{
  Start-Sleep -Milliseconds $timeoutStepMilliseconds
  $timeoutCounter += $timeoutStepMilliseconds
  Write-Progress -Activity "Sending Command $Command" -Status "Receiving response $timeoutCounter ms"
  if ($timeoutCounter -gt $ReceiveTimeoutMilliseconds) { throw "Waiting for ReceiveAsync timed out" }
}
Write-Progress -Activity "Sending Command $Command" -Status "Received"

# For a better way to handle close, and to receive buffer chunks
# See: https://stackoverflow.com/questions/28330705/how-to-gracefully-close-a-two-way-websocket-in-net

# Close

$timeoutCounter = 0;
Write-Progress -Activity "Sending Command $Command" -Status "Closing"
Write-Verbose "Closing"

$close = $webSocket.CloseAsync("NormalClosure", "", $ct)

while (-not $close.IsCompleted)
{
  Start-Sleep -Milliseconds $timeoutStepMilliseconds
  $timeoutCounter += $timeoutStepMilliseconds
  Write-Progress -Activity "Sending Command $Command" -Status "Closing $timeoutCounter ms"
  if ($timeoutCounter -gt $TimeoutMilliseconds) { throw "Waiting for CloseAsync timed out" }
}
Write-Progress -Activity "Sending Command $Command" -Status "Closed"

# Output response:

$json = [System.Text.Encoding]::UTF8.GetString($buffer)
$response = ConvertFrom-Json $json

Write-Progress -Activity "Sending Command $Command" -Completed

Write-Output $response
