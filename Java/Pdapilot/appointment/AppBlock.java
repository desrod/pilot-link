
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
	
	void fill() {
		startOfWeek = 0;
	}
		
	public String describe() {
		return "startOfWeek="+ startOfWeek +", " + super.describe();
	}
}
