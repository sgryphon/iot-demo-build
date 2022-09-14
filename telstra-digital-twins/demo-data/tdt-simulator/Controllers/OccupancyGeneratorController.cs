using Microsoft.AspNetCore.Mvc;

namespace tdt_simulator.Controllers;

// http://localhost:5000/OccupancyGenerator?api_key=hV9N1J4vDY46h19r

[ApiController]
[Route("[controller]")]
public class OccupancyGeneratorController : ControllerBase
{
    private const string ExpectedApiKey = "hV9N1J4vDY46h19r";
    private const double OccupancyFactor = 0.4;
    private static readonly Occupancy Occupancy = new Occupancy(
        new List<Workspace> { 
            new Workspace("W.9.001", "242-exhibition-melb", "Level-9"),
            new Workspace("W.9.002", "242-exhibition-melb", "Level-9"),
            new Workspace("W.9.003", "242-exhibition-melb", "Level-9"),
            new Workspace("W.9.001", "242-exhibition-melb", "Level-9")
        }
    );

    private readonly ILogger<OccupancyGeneratorController> _logger;

    public OccupancyGeneratorController(ILogger<OccupancyGeneratorController> logger)
    {
        _logger = logger;
    }

    [HttpGet(Name = "GetOccupancy")]
    public ActionResult<Occupancy> Get()
    {
        string apiKey = string.Empty;
        if (HttpContext.Request.Headers.TryGetValue("X-API-Key", out var apiKeyHeaders)) {
            apiKey = apiKeyHeaders.First();
        } else {
            if (HttpContext.Request.Query.TryGetValue("api_key", out var apiKeyValues)) {
                apiKey = apiKeyValues.First();
            }
        }
        if (apiKey != ExpectedApiKey) {
            return Unauthorized();
        }

        foreach (var workspace in Occupancy.Workspaces) {
            workspace.Occupied = Random.Shared.NextDouble() < OccupancyFactor;
        }
        return Occupancy;
    }
}
