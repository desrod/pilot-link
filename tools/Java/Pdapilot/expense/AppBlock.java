/* 
 * expense/AppBlock.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
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
