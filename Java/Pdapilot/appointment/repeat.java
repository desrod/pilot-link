/* appointment/repeat.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.appointment;

public class repeat {
	private int idx;
	
	final static private String[] names = 
		{ "None", "Daily", "Weekly", "MonthlyByDay", "MonthlyByDate", "Yearly" };

	public static repeat None = new repeat(0);
	public static repeat Daily = new repeat(1);
	public static repeat Weekly = new repeat(2);
	public static repeat MonthlyByDay = new repeat(3);
	public static repeat MonthlyByDate = new repeat(4);
	public static repeat Yearly = new repeat(5);
	
	private static repeat[] objs;
	
	private repeat(int value) {
		this.idx = value;
	}
	
	public static repeat get(int value) {
		return objs[value];
	}
	public static repeat get(String value) {
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
		return "repeat."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new repeat[6];
		objs[0] = repeat.None;
		objs[1] = repeat.Daily;
		objs[2] = repeat.Weekly;
		objs[3] = repeat.MonthlyByDay;
		objs[4] = repeat.MonthlyByDate;
		objs[5] = repeat.Yearly;
	}
};