/* 
 * expense/Pref.java:
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

package Pdapilot.expense;

import java.io.*;

public class Pref extends Pdapilot.Pref {
	public int currentCategory, defaultCategory;
	public int noteFont;
	public boolean showAllCategories;
	public boolean showCurrency;
	public boolean saveBackup;
	public boolean allowQuickFill;
	public distance unitOfDistance;
	public int[] currencies;
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	                
		
		public Pref() {
			super(null, new Pdapilot.Char4("exps"), 1, 1, true);
		}

		public Pref(byte[] contents, Pdapilot.Char4 creator, int id, int version, boolean backup) {
			super(contents, creator, id, version, backup);
		}
		
		public String describe() {
			return super.describe();
			/*return "syncType="+syncType+", getHigh=" +getHigh+", getContaining="+getContaining+", "+
			       "truncate="+truncate+", filterTo="+Pdapilot.Util.prettyPrint(filterTo)+
			       ", filterFrom="+Pdapilot.Util.prettyPrint(filterFrom)+
			       ", filterSubject="+Pdapilot.Util.prettyPrint(filterSubject)+", "+super.describe();*/
		}
}
