/* 
 * todo/Record.java:
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
