
package Pdapilot.address;

import java.io.*;

public class AppBlock extends Pdapilot.CategoryAppBlock {
	public boolean sortByCompany;
	public int country;
	public String[] label;
	public String[] phoneLabel;
	public int[] labelRenamed;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	
	public void fill() {
		sortByCompany = false;
		country = 0;
		label = new String[22];
		phoneLabel = new String[8];
	}
		
	public String describe() {
		StringBuffer c = new StringBuffer("labels=[");
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
		c.append("], labelRenamed=[");
		for(int i=0;i<labelRenamed.length;i++) {
			if (i>0)
				c.append(",");
			c.append(labelRenamed[i]);
		}
		c.append("], sortByCompany="+sortByCompany+", country="+country);
		return "" + c + ", " + super.describe();
	}
}
