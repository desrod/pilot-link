/* 
 * mail/Record.java:
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

/** A representation of a mail database record.
 */

public class Record extends Pdapilot.Record {

		public boolean read, signature, confirmRead, confirmDelivery;
		public int priority, addressing;
		
		public java.util.Date date;
		public String subject, from, to, cc, bcc, replyTo, sentTo, body;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
		}
		
        public String describe() {
        	StringBuffer c = new StringBuffer("read=");
          return "read="+read+", signature="+signature+", confirmRead="+confirmRead+
                 ", confirmDelivery="+confirmDelivery+", priority="+priority+
                 ", addressing="+addressing+", date="+((date == null)?"<Null":date.toString())+
                 ", from="+Pdapilot.Util.prettyPrint(from)+
                 ", to="+Pdapilot.Util.prettyPrint(to)+
                 ", cc="+Pdapilot.Util.prettyPrint(cc)+
                 ", bcc="+Pdapilot.Util.prettyPrint(bcc)+
                 ", replyTo="+Pdapilot.Util.prettyPrint(replyTo)+
                 ", sentTo="+Pdapilot.Util.prettyPrint(sentTo)+
                 ", body="+Pdapilot.Util.prettyPrint(body)+
                 ", subject="+Pdapilot.Util.prettyPrint(subject)+
                 ", "+super.describe();
        }
}
