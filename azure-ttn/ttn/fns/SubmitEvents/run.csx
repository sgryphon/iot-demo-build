#r "System.Text.Json"
#r "Microsoft.Azure.EventHubs"
#r "Microsoft.Azure.WebJobs"
#r "Microsoft.Extensions.Configuration"
#r "Microsoft.Extensions.Configuration.Abstractions"
#r "Microsoft.Extensions.Configuration.FileExtensions"
#r "Microsoft.Extensions.Configuration.EnvironmentVariables"

using System.Threading.Tasks;
using CToken = System.Threading.CancellationToken;
using System.Text.Json.Serialization;
using System.Text.Json;
using System.Text;
using System.Net.Http;
using System.IO;
using System.Collections.Generic;
using System;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Configuration;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.EventHubs;

public struct ApplicationIdentifiers
{
    [JsonPropertyName("application_id")]
    public string ApplicationID { get; set; }

    public ApplicationIdentifiers(string applicationID) => ApplicationID = applicationID;
}

public struct EndDeviceIdentifiers
{
    [JsonPropertyName("application_ids")]
    public ApplicationIdentifiers ApplicationIdentifiers { get; set; }

    [JsonPropertyName("device_id")]
    public string DeviceID { get; set; }

    public EndDeviceIdentifiers(ApplicationIdentifiers applicationIdentifiers, string deviceID)
    {
        ApplicationIdentifiers = applicationIdentifiers;
        DeviceID = deviceID;
    }
}

public struct StackEvent
{
    [JsonPropertyName("end_device_ids")]
    public EndDeviceIdentifiers EndDeviceIdentifiers { get; set; }

    [JsonPropertyName("body")]
    public JsonElement Body { get; set; }

    [JsonPropertyName("properties")]
    public IDictionary<string, object> Properties { get; set; }

    public StackEvent(EndDeviceIdentifiers endDeviceIdentifiers, JsonElement body, IDictionary<string, object> properties)
    {
        EndDeviceIdentifiers = endDeviceIdentifiers;
        Body = body;
        Properties = properties;
    }
}

public struct StackEvents
{
    [JsonPropertyName("events")]
    public StackEvent[] Events { get; set; }

    public StackEvents(StackEvent[] events) => Events = events;
}

private static readonly HttpClient client = new HttpClient();

[FunctionName("SubmitEvents")]
public static async Task Run(EventData[] events, ExecutionContext context, ILogger log, CToken token)
{
    var config = new ConfigurationBuilder().SetBasePath(context.FunctionAppDirectory).AddEnvironmentVariables().Build();

    var applicationIdentifiers = new ApplicationIdentifiers(config["STACK_APPLICATION_ID"]);
    var apiKey = config["STACK_API_KEY"];
    var uriBuilder = new UriBuilder(config["STACK_BASE_URL"]);
    uriBuilder.Path = Path.Combine(uriBuilder.Path, "as/applications", applicationIdentifiers.ApplicationID, "packages/azureiothub/events");

    var stackEvents = new StackEvent[events.Length];
    for (var i = 0; i < events.Length; i++)
    {
        var ev = events[i];
        var props = ev.Properties;
        var deviceID = (string)props["deviceId"];

        stackEvents[i] = new StackEvent(
            new EndDeviceIdentifiers(applicationIdentifiers, deviceID),
            JsonDocument.Parse(ev.Body).RootElement,
            props
        );
    }

    var response = await client.SendAsync(new HttpRequestMessage
    {
        RequestUri = uriBuilder.Uri,
        Method = HttpMethod.Post,
        Headers = {
                    {"Authorization", $"Bearer {apiKey}"}
                },
        Content = new StringContent(
            JsonSerializer.Serialize(new StackEvents(stackEvents)),
            Encoding.UTF8,
            "application/json"
        ),
    }, token);
    response.EnsureSuccessStatusCode();
}
