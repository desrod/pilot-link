/* expense/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.expense;

import java.io.*;
import Pdapilot.Util;

public class AppBlock extends Pdapilot.CategoryAppBlock {
	
	public sort sortOrder;
	public CustomCurrency[] currencies;
		
	public AppBlock() {
		super();
	}
		
	public AppBlock(byte[] contents) {
		super(contents);
	}
	
	public native void unpack(byte[] data);
	public native byte[] pack();
	
	public void fill() {
		currencies = new CustomCurrency[4];
	}
		
	public String describe() {
		StringBuffer c = new StringBuffer();
		c.append("currencies=[");
		if (currencies != null)
			for (int i=0;i<currencies.length;i++) {
				if (i>0)
					c.append(",");
				c.append("[");
				c.append("name=" + Util.prettyPrint(currencies[i].name));
				c.append(",symbol=" + Util.prettyPrint(currencies[i].symbol));
				c.append(",rate=" + Util.prettyPrint(currencies[i].rate));
				c.append("]");
			}
		c.append("]");
		return "sort="+ sortOrder+", " + c + ", " + super.describe();
	}
}
