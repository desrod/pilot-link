/* expense/Record.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.expense;

import java.io.*;
import Pdapilot.Util;

/** A representation of an expense database record.
 */

public class Record extends Pdapilot.Record {

		public java.util.Date date;
		public String amount, vendor, city, attendees, note;
		public int currency;
		public type type;
		public payment payment;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
			type = type.Telephone;
			payment = payment.Cash;
		}
		
        public String describe() {
        	return 
        	"date="+Util.prettyPrint(date)+
        	", currency="+currency+
        	", type="+type+
        	", payment="+payment+
        	", amount="+Util.prettyPrint(amount)+
        	", vendor="+Util.prettyPrint(vendor)+
        	", city="+Util.prettyPrint(city)+
        	", attendees="+Util.prettyPrint(attendees)+
        	", note="+Util.prettyPrint(note)+
        	", "+super.describe();
        }
}
