
package Pdapilot;

public class CardInfo {
	public int card;
	public int version;
	public java.util.Date creation;
	public int romSize, ramSize, ramFree;
	public String name;
	public String manufacturer;
	
	public boolean more;
	
	public String toString() {
		return "<Pdapilot.CardInfo card "+card+", version "+version+", romSize "+romSize+", ramSize "+ramSize+
			", ramFree "+ramFree+", name '"+name+"', manufacturer '"+manufacturer+"', created "+((creation == null) ? "null" : creation.toString())+">";
	}
}

