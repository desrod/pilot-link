
package Pdapilot;

import java.io.*;

/** A representation of a database record.
 */

public class Record extends Block {
		public int index, attr, cat;
		public RecordID id;
		
		public Record() {
			super();
		}
		
		public Record(byte[] contents, RecordID id, int index, int attr, int cat) {
			this.id = id;
			this.index = index;
			this.attr = attr;
			this.cat = cat;
			this.unpack(contents);
		}
		
		public String toString() {
			return "<generic record, id "+id+", index "+index+", attr "+attr+", cat "+
				cat+", raw '"+Util.prettyPrint(new String(raw,0))+"'>";
		}
}
