/* Pref.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

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
		
		public String describe() {
			return " creator="+creator.toString()+", id="+id+", version="+version+
				", backup="+backup+", raw='"+Util.prettyPrint(raw)+"'";
		}
}
