/* mail/Record.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
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
