/* memo/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.memo;

import java.io.*;


public class AppBlock extends Pdapilot.CategoryAppBlock {
	boolean sortByAlpha;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
		
	public String describe() {
		return "sortByAlpha" + sortByAlpha + ", " + super.describe();
	}
	public String toString() {
		return "<" + this.getClass().getName() + " " + describe() + ">";
	}
}
