/* 
 * Record.java:
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
