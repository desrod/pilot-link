/* expense/sort.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.expense;

public class sort {
	private int idx;
	
	final static private String[] names = 
		{ "Date", "Type" };

	public static sort Date = new sort(0);
	public static sort Type =  new sort(1);
	
	private static sort[] objs;
	
	private sort(int value) {
		this.idx = value;
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

	static {
		objs = new sort[2];
		objs[0] = sort.Date;
		objs[1] = sort.Type;
	}
};