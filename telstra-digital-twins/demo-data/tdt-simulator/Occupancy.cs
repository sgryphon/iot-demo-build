namespace tdt_simulator;

public class Occupancy
{
    public Occupancy(IList<Workspace> workspaces) {
        Workspaces = new List<Workspace>(workspaces);
    }
    
    public IList<Workspace> Workspaces { get; set; }
}
