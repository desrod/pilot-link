/* SortBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;


public class SortBlock extends Block {
		
		public SortBlock() {
			super();
		}
		
		public SortBlock(byte[] contents) {
			super(contents);
		}
		
		public String toString() {
			return "<generic sortblock raw '"+Util.prettyPrint(raw)+"'>";
		}
}
