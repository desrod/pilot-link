/* Resource.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;

/** A representation of a database resource.
 */

public class Resource extends Block {
		public int id, index;
		public Char4 type;
		
		Resource() {
			super();
		}
		
		Resource(byte[] contents, Char4 type, int id, int index) {
			this.id = id;
			this.index = index;
			this.type = type;
			this.unpack(contents);
		}

		public String toString() {
			return "<generic resource, type "+type+", id "+id+", index "+index+
				", raw '"+Util.prettyPrint(raw)+"'>";
		}
}
