
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
