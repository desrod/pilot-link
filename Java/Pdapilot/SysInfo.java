
package Pdapilot;

public class SysInfo {
	public int romVersion;
	public int locale;
	public String name;
	
	public String toString() {
		return "<Pdapilot.SysInfo ROMversion "+romVersion+", locale "+locale+
			", name '"+Util.prettyPrint(name)+"'>";
	}
}
