/* 
 * mail/sort.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

package Pdapilot.mail;

public class sort {
	private int idx;
	
	final static private String[] names = new String[2];
	private static sort[] objs = new sort[2];

	public static sort Date = new sort(0, "Date");
	public static sort Type =  new sort(1, "Type");
	
	private sort(int value, String name) {
		this.idx = value;
		objs[value] = this;
		names[value] = name;
	}
	
	public static sort get(int value) {
		return objs[value];
	}
	public static sort get(String value) {
		int i;
		for(i=0;i<names.length;i++)
			if (names[i].equals(value))
				return objs[i];
		return null;
	}
	public static String[] getNames() {
		return names;
	}
	
	public String toString() {
		return "sort."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}

	public String getName() {
		return names[idx];
	}
};
