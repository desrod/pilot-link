
package Pdapilot;

import java.io.*;

/** A representation of a database record.
 */

public class Record extends Block {
		public int index, attr, category;
		public RecordID id;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, RecordID id, int index, int attr, int category) {
			this.id = id;
			this.index = index;
			this.attr = attr;
			this.category = category;
			this.unpack(contents);
		}
		
		public String describe() {
			return " id "+id+", index "+index+", attr "+attr+", category "+
				category;
		}
}
