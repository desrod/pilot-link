/* mail/sort.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
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