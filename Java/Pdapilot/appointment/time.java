/* appointment/time.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.appointment;

public class time {
	private int idx;
	
	final static private String[] names = 
		{ "Minutes", "Hours", "Days" };

	public static time Minutes = new time(0);
	public static time Hours =  new time(1);
	public static time Days = new time(2);
	
	private static time[] objs;
	
	private time(int value) {
		this.idx = value;
	}
	
	public static time get(int value) {
		return objs[value];
	}
	public static time get(String value) {
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
		return "time."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new time[3];
		objs[0] = time.Minutes;
		objs[1] = time.Hours;
		objs[2] = time.Days;
	}
};