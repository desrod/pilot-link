
package Pdapilot.mail;

import java.io.*;

public class AppBlock extends Pdapilot.CategoryAppBlock {
	
	public int sortOrder;
	public Pdapilot.RecordID unsentMessage;
	public int dirty;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	
	void fill() {
	}
		
	public String describe() {
		/*StringBuffer c = new StringBuffer("labels=[");
		for(int i=0;i<label.length;i++) {
			if (i>0)
				c.append(",");
			c.append(label[i]);
		}
		c.append("], phoneLabel=[");
		for(int i=0;i<phoneLabel.length;i++) {
			if (i>0)
				c.append(",");
			c.append(phoneLabel[i]);
		}
		c.append("], dirty="+dirty+", sortByCompany="+sortByCompany+", country="+country);
		return "" + c + ", " + super.describe();*/
		return super.describe();
	}
}
