/* expense/CustomCurrency.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.expense;

public class CustomCurrency {
	public String name, symbol, rate;
	
	public String toString() {
		return "<Pdapilot.expense.CustomCurrency name="+name+", symbol="+symbol+
			", rate="+rate+">";
	}
}
