/* mail/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.mail;

import java.io.*;

public class AppBlock extends Pdapilot.CategoryAppBlock {
	
	public sort sortOrder;
	public Pdapilot.RecordID unsentMessage;
	public boolean dirty;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	
	public void fill() {
	}
		
	public String describe() {
		return "sort="+sortOrder+", unsentMessage="+unsentMessage+
				", dirty="+dirty+", "+super.describe();
	}
}
