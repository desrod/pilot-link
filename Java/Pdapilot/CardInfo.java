/* 
 * CardInfo.java:  Class to describe "cards" on the Palm device.
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
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

