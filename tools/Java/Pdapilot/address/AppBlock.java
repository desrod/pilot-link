/* 
 * address/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

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
