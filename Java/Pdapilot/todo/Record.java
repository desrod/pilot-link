
package Pdapilot.todo;

import java.io.*;
import Pdapilot.Util;

/** A representation of a todo database record.
 */

public class Record extends Pdapilot.Record {

		public String description, note;
		public java.util.Date due;
		public boolean complete;
		public int priority;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
			priority = 0;
			complete = false;
			description = "";
			due = null;
		}
		
        public String describe() {
            return  "description '"+ Util.prettyPrint(description)+
          ", note '"+ Util.prettyPrint(note)+
          ", due "+ Util.prettyPrint(due) +
          ", complete " + complete +
          ", priority " + priority + "', "+super.describe();
        }
}
