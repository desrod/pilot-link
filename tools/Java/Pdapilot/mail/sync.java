/* 
 * mail/sync.java:
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

package Pdapilot.mail;

public class sync {
	private int idx;
	
	final static private String[] names = 
		{ "All", "Send", "Filter" };

	public static sync All = new sync(0);
	public static sync Send =  new sync(1);
	public static sync Filter = new sync(2);
	
	private static sync[] objs;
	
	private sync(int value) {
		this.idx = value;
	}
	
	public static sync get(int value) {
		return objs[value];
	}
	public static sync get(String value) {
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
		return "sync."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new sync[3];
		objs[0] = sync.All;
		objs[1] = sync.Send;
		objs[2] = sync.Filter;
	}
};
