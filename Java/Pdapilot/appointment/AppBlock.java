/* appointment/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.appointment;

import java.io.*;

public class AppBlock extends Pdapilot.CategoryAppBlock {
	public int startOfWeek;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	
	public void fill() {
		startOfWeek = 0;
	}
		
	public String describe() {
		return "startOfWeek="+ startOfWeek +", " + super.describe();
	}
}
