/* todo/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

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
