/* address/Record.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.address;

import java.io.*;

/** A representation of an address database record.
 */

public class Record extends Pdapilot.Record {

		public int[] phoneLabel;
		public int showPhone;
		public String[] entry;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
			entry = new String[19];
			phoneLabel = new int[5];
			showPhone = 0;
		}
		
        public String describe() {
        	StringBuffer c = new StringBuffer("phoneLabel=[");
        	for(int i=0;i<phoneLabel.length;i++) {
        		if (i>0)
        			c.append(",");
        		c.append(phoneLabel[i]);
        	}
        	c.append("], entry=[");
        	for(int i=0;i<entry.length;i++) {
        		if (i>0)
        			c.append(",");
        		c.append((entry[i] == null) ? "(null)" : entry[i]);
        	}
        	c.append("]");
          return "showPhone="+showPhone+", "+c+", "+super.describe();
        }
}
