
package Pdapilot;

public class CardInfo {
	public int number;
	public int version;
	public java.util.Date creation;
	public int ROMsize, RAMsize, RAMfree;
	public String name;
	public String manufacturer;
	
	public boolean more;
	
	public String toString() {
		return "<Pdapilot.CardInfo number "+number+", version "+version+", ROMsize "+ROMsize+", RAMsize "+RAMsize+
			", RAMfree "+RAMfree+", name '"+name+"', manufacturer '"+manufacturer+"', created "+((creation == null) ? "null" : creation.toString())+">";
	}
}

