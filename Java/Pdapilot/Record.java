/* Record.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;

/** A representation of a database record.
 */

public class Record extends Block {
		public int index, category;
		public RecordID id;
		public boolean deleted, modified, busy, secret, archived;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, RecordID id, int index, int attr, int category) {
			this.id = id;
			this.index = index;
			this.deleted = ((attr & 0x80)!=0) ? true : false;
			this.modified = ((attr & 0x40)!=0) ? true : false;
			this.busy = ((attr & 0x20)!=0) ? true : false;
			this.secret = ((attr & 0x10)!=0) ? true : false;
			this.archived = ((attr & 0x08)!=0) ? true : false;
			this.category = category;
			this.unpack(contents);
		}
		
		public void Fill() {
			this.deleted = false;
			this.modified = false;
			this.busy = false;
			this.secret = false;
			this.archived = false;
		}
		
		public String describe() {
			return " id "+id+", index "+index+", deleted "+deleted+", modified "+modified+
			", busy "+busy+", secret "+secret+", archived "+archived+", category "+ category;
		}
}
