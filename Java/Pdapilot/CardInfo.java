/* CardInfo.java:  Class to describe "cards" on the Palm device.
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

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

