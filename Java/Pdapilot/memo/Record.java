/* memo/Record.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.memo;

import java.io.*;

/** A representation of a memo database record.
 */

public class Record extends Pdapilot.Record {

		public String text;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
			text = "";
		}
		
        public String describe() {
            return "text '"+Pdapilot.Util.prettyPrint(text)+"', "+super.describe();
        }
}
