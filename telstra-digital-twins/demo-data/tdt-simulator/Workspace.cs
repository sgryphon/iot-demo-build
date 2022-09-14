namespace tdt_simulator;

public class Workspace
{
    public Workspace(string id, string buildingName, string levelName, bool occupied = false)
    {
        Id = id;
        BuildingName = buildingName;
        LevelName = levelName;
    }
    
    public string BuildingName { get; set; }
    public string LevelName { get; set; }
    public string Id { get; set; }
    public bool Occupied  { get; set; }
}
