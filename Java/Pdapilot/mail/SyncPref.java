/* 
 * mail/SyncPref.java:
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

package Pdapilot.mail;

import java.io.*;

public class SyncPref extends Pdapilot.Pref {
	public sync syncType;
	public boolean getHigh;
	public boolean getContaining, truncate;
	public String filterTo, filterFrom, filterSubject;
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	                
		
		public SyncPref() {
			super(null, new Pdapilot.Char4("mail"), 1, 1, true);
		}

		public SyncPref(boolean local) {
			super(null, new Pdapilot.Char4("mail"), local ? 1 : 2, 1, true);
		}
		
		public SyncPref(byte[] contents, Pdapilot.Char4 creator, int id, int version, boolean backup) {
			super(contents, creator, id, version, backup);
		}
		
		public String describe() {
			return "syncType="+syncType+", getHigh=" +getHigh+", getContaining="+getContaining+", "+
			       "truncate="+truncate+", filterTo="+Pdapilot.Util.prettyPrint(filterTo)+
			       ", filterFrom="+Pdapilot.Util.prettyPrint(filterFrom)+
			       ", filterSubject="+Pdapilot.Util.prettyPrint(filterSubject)+", "+super.describe();
		}
}
