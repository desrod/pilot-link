
package Pdapilot.todo;

import java.io.*;


public class AppBlock extends Pdapilot.AppBlock {
	int renamedCategories;
	int lastUniqueID;
	boolean sortByPriority;
	boolean dirty;
	String[] categoryName;
	int[] categoryID;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
		
	public String toString() {
		StringBuffer c;
		if ((categoryName == null) || (categoryID == null)) {
			c = new StringBuffer("no categories");
		} else {
			c = new StringBuffer("categories:");
			for (int i=0;i<categoryName.length;i++) {
				c.append(" ");
				c.append(categoryName[i]);
				c.append("/");
				c.append(categoryID[i]);
			}
		}
		return "<memo appblock renamedCategories="+renamedCategories+
				", lastUniqueID="+lastUniqueID+
				", sortByPriority="+sortByPriority+
				", dirty="+dirty+
				", " + c + 
				">";
	}
}
