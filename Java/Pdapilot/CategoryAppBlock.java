/* CategoryAppBlock.java:  Class describing AppBlocks.
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

import java.io.*;

public class CategoryAppBlock extends AppBlock {
	public boolean[] categoryRenamed;
	public int categoryLastUniqueID;
	public String[] categoryName;
	public int[] categoryID;
		
	public CategoryAppBlock() {
		super();
	}

	public native void unpack(byte[] data);
	public native byte[] pack();
	
	public void fill() {
		categoryRenamed = new boolean[16];
		categoryLastUniqueID = 0;
		categoryName = new String[16];
		categoryID = new int[16];
	}
		
	public CategoryAppBlock(byte[] contents) {
		super(contents);
	}
		
	public String describe() {
		StringBuffer c;
		if ((categoryName == null) || (categoryID == null) || (categoryRenamed == null)) {
			c = new StringBuffer("no categories");
		} else {
			c = new StringBuffer("name             id\trenamed\n");
			for (int i=0;i<categoryName.length;i++) {
				if (categoryName[i].length() > 0) {
					c.append(categoryName[i]);
					for(int j=16;j>categoryName[i].length();j--)
						c.append(" ");
					c.append(" ");
					c.append(categoryID[i]);
					c.append("\t");
					c.append(categoryRenamed[i]);
					c.append("\n");
				}
			}
		}
		return "categoryLastUniqueID="+categoryLastUniqueID+
				"\n" + c;
	}
}

		
