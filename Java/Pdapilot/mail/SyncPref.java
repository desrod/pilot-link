/* mail/SyncPref.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
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
