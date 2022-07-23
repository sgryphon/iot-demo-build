#!/usr/bin/env pwsh

<# .SYNOPSIS
  Add twin instances, based on the factory ontology
#>
[CmdletBinding()]
param (
    ## Deployment environment, e.g. Prod, Dev, QA, Stage, Test.
    [string]$Environment = 'Dev',
    ## Identifier for the organisation (or subscription) to make global names unique.
    [string]$OrgId = $ENV:DEPLOY_ORGID ?? "0x$((az account show --query id --output tsv).Substring(0,4))"
)

$ErrorActionPreference="Stop"

$SubscriptionId = $(az account show --query id --output tsv)
Write-Verbose "Using subscription ID $SubscriptionId"

$appName = 'iotcore'
$dtName = "dt-$appName-$OrgId-$Environment".ToLowerInvariant()

Write-Verbose "Adding factory twin instances to $dtName"

Write-Verbose "Factory, Site, and Area"

az dt twin create --dt-name $dtName --dtmi "dtmi:digitaltwins:s4inma:Factory;1" --twin-id "factory-01" --properties '{""label"":""Factory 01""}'

az dt twin create --dt-name $dtName --dtmi "dtmi:digitaltwins:s4inma:Site;1" --twin-id "site-01-01" --properties '{""label"":""Site 01""}'
az dt twin relationship create --dt-name $dtName --relationship-id "hasSpace_site-01-01" --kind "hasSpace" --twin-id "factory-01" --target "site-01-01"

az dt twin create -n $dtName -m "dtmi:digitaltwins:s4inma:Area;1" -t "area-01-01-01" -p '{""label"":""Area 01""}'
az dt twin relationship create -n $dtName -r "hasSpace_area-01-01" --kind "hasSpace" -t "site-01-01" --target "area-01-01-01"

Write-Verbose "Production line 1"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:ProductionLine;1" -t "line-001" -p '{""label"":""Line 001"",""hasManufacturer"":""Acme"",""hasModel"":""Chocolate Maker Extreme"",""ProductBatchIdentifier"":""B.001.000001"",""ConveyorSpeed"":2.5}'
az dt twin relationship create -n $dtName -r "contains_line-001" --kind "contains" -t "area-01-01-01" --target "line-001"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:RoastingEquipment;1" -t "roaster-001" -p '{""label"":""Roaster 001"",""hasManufacturer"":""Acme"",""hasModel"":""Cocoa Roaster Basic"",""ChasisTemperature"":36.0,""FanSpeed"":20.0}'
az dt twin relationship create -n $dtName -r "consistsOf_roaster-001" --kind "consistsOf" -t "line-001" --target "roaster-001"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:GrindingEquipment;1" -t "grinder-001" -p '{""label"":""Grinder 001"",""hasManufacturer"":""Acme"",""hasModel"":""Fast Grinder 5"",""Force"":10.0,""Vibration"":98.7}'
az dt twin relationship create -n $dtName -r "consistsOf_grinder-001" --kind "consistsOf" -t "line-001" --target "grinder-001"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:MouldingEquipment;1" -t "moulder-001" -p '{""label"":""Moulder 001"",""hasManufacturer"":""Acme"",""hasModel"":""UltraMould 23"",""ChasisTemperature"":23.5}'
az dt twin relationship create -n $dtName -r "consistsOf_moulder-001" --kind "consistsOf" -t "line-001" --target "moulder-001"

Write-Verbose "Production line 2"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:ProductionLine;1" -t "line-002" -p '{""label"":""Line 002"",""hasManufacturer"":""Acme"",""hasModel"":""Chocolate Maker Extreme 2"",""ProductBatchIdentifier"":""B.002.000001"",""ConveyorSpeed"":2.7}'
az dt twin relationship create -n $dtName -r "contains_line-002" --kind "contains" -t "area-01-01-01" --target "line-002"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:RoastingEquipment;1" -t "roaster-002" -p '{""label"":""Roaster 002"",""hasManufacturer"":""Acme"",""hasModel"":""Cocoa Roaster Plus"",""ChasisTemperature"":35.0,""FanSpeed"":18.0}'
az dt twin relationship create -n $dtName -r "consistsOf_roaster-002" --kind "consistsOf" -t "line-002" --target "roaster-002"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:GrindingEquipment;1" -t "grinder-002" -p '{""label"":""Grinder 002"",""hasManufacturer"":""Acme"",""hasModel"":""Fast Grinder 5"",""Force"":9.8,""Vibration"":101.2}'
az dt twin relationship create -n $dtName -r "consistsOf_grinder-002" --kind "consistsOf" -t "line-002" --target "grinder-002"

az dt twin create -n $dtName -m "dtmi:digitaltwins:factory:MouldingEquipment;1" -t "moulder-002" -p '{""label"":""Moulder 002"",""hasManufacturer"":""Acme"",""hasModel"":""UltraMould 42"",""ChasisTemperature"":20.0}'
az dt twin relationship create -n $dtName -r "consistsOf_moulder-002" --kind "consistsOf" -t "line-002" --target "moulder-002"
