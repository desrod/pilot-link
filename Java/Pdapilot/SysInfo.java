
package Pdapilot;

public class SysInfo {
	public int ROMversion;
	public int localeID;
	public String name;
	
	public String toString() {
		return "<Pdapilot.SysInfo ROMversion "+ROMversion+", localeID "+localeID+
			", name '"+Util.prettyPrint(name)+"'>";
	}
}
