
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
			return "raw='"+Util.prettyPrint(new String(raw,0))+"'";
		}
}
