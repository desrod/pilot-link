
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
