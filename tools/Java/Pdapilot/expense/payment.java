/* 
 * expense/payment.java:
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

public class payment {
	private int idx;
	
	final static private String[] names = 
		{ "AmEx", "Cash", "Check", "Credit Card", "MasterCard", "Prepaid", "VISA", "Unfiled" };

	public static payment AmEx = new payment(0);
	public static payment Cash =  new payment(1);
	public static payment Check =  new payment(2);
	public static payment CreditCard =  new payment(3);
	public static payment MasterCard =  new payment(4);
	public static payment Prepaid =  new payment(5);
	public static payment VISA =  new payment(6);
	public static payment Unfiled =  new payment(7);
	
	private static payment[] objs;
	
	private payment(int value) {
		this.idx = value;
	}
	
	public static payment get(int value) {
		return objs[value];
	}
	public static payment get(String value) {
		int i;
		for(i=0;i<names.length;i++)
			if (names[i].equals(value))
				return objs[i];
		return null;
	}
	public static String[] getNames() {
		return names;
	}
	
	public String toString() {
		return "payment."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new payment[8];
		objs[0] = payment.AmEx;
		objs[1] = payment.Cash;
		objs[2] = payment.Check;
		objs[3] = payment.CreditCard;
		objs[4] = payment.MasterCard;
		objs[5] = payment.Prepaid;
		objs[6] = payment.VISA;
		objs[7] = payment.Unfiled;
	}
};
