
package Pdapilot;

import java.io.*;

public class Pref extends Block {
		public int id, version;
		public boolean backup;
		public Char4 creator;
		
		public Pref() {
			super();
		}
		
		public Pref(byte[] contents, Char4 creator, int id, int version, boolean backup) {
			this.id = id;
			this.creator = creator;
			this.version = version;
			this.backup = backup;
			this.unpack(contents);
		}
		
		public String toString() {
			return "<generic pref, creator="+creator.toString()+", id="+id+", version="+version+
				", backup="+backup+", raw='"+Util.prettyPrint(raw)+"'>";
		}
}
