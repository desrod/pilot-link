/* 
 * expense/Record.java:
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
