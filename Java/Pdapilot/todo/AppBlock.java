
package Pdapilot.todo;

import java.io.*;


public class AppBlock extends Pdapilot.CategoryAppBlock {
	boolean sortByPriority;
	boolean dirty;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
		
	public String describe() {
		return "sortByPriority="+sortByPriority+ ", dirty=" + dirty + ", " + super.describe();
	}
}
