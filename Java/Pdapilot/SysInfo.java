/* SysInfo.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

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
