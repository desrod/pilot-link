
/* AppBlock.java:  Base class for application blocks. (Almost abstract)
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;


public class AppBlock extends Block {
		
		public AppBlock() {
			super();
		}
		
		public AppBlock(byte[] contents) {
			super(contents);
		}
		
		public String describe() {
			return "raw='"+Util.prettyPrint(raw)+"'";
		}
}
