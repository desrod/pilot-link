/* expense/type.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.expense;

public class type {
	private int idx;
	
	final static private String[] names = 
		{ "Airfare", "Breakfast", "Bus", "Business Meals", "Car Rental", 
		  "Dinner", "Entertainment", "Fax", "Gas", "Gifts", "Hotel",
		  "Incidentals", "Laundry", "Limo", "Lodging", "Lunch", "Mileage",
		  "Other", "Parking", "Postage", "Snack", "Subway", "Supplies",
		  "Taxi", "Telephone", "Tips", "Tolls", "Train" };

	public static type Airfare = new type(0);
	public static type Breakfast =  new type(1);
	public static type Bus =  new type(2);
	public static type BusinessMeals =  new type(3);
	public static type CarRental =  new type(4);
	public static type Dinner =  new type(5);
	public static type Entertainment =  new type(6);
	public static type Fax =  new type(7);
	public static type Gas =  new type(8);
	public static type Gifts =  new type(9);
	public static type Hotel =  new type(10);
	public static type Incidentals =  new type(11);
	public static type Laundry =  new type(12);
	public static type Limo =  new type(13);
	public static type Lodging =  new type(14);
	public static type Lunch =  new type(15);
	public static type Mileage =  new type(16);
	public static type Other =  new type(17);
	public static type Parking =  new type(18);
	public static type Postage =  new type(19);
	public static type Snack =  new type(20);
	public static type Subway =  new type(21);
	public static type Supplies =  new type(22);
	public static type Taxi =  new type(23);
	public static type Telephone =  new type(24);
	public static type Tips =  new type(25);
	public static type Tolls =  new type(26);
	public static type Train =  new type(27);
	
	private static type[] objs;
	
	private type(int value) {
		this.idx = value;
	}
	
	public static type get(int value) {
		return objs[value];
	}
	public static type get(String value) {
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
		return "type."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new type[28];
		objs[0] = type.Airfare;
		objs[1] = type.Breakfast;
		objs[2] = type.Bus;
		objs[3] = type.BusinessMeals;
		objs[4] = type.CarRental;
		objs[5] = type.Dinner;
		objs[6] = type.Entertainment;
		objs[7] = type.Fax;
		objs[8] = type.Gas;
		objs[9] = type.Gifts;
		objs[10] = type.Hotel;
		objs[11] = type.Incidentals;
		objs[12] = type.Laundry;
		objs[13] = type.Limo;
		objs[14] = type.Lodging;
		objs[15] = type.Lunch;
		objs[16] = type.Mileage;
		objs[17] = type.Other;
		objs[18] = type.Parking;
		objs[19] = type.Postage;
		objs[20] = type.Snack;
		objs[21] = type.Subway;
		objs[22] = type.Supplies;
		objs[23] = type.Taxi;
		objs[24] = type.Telephone;
		objs[25] = type.Tips;
		objs[26] = type.Tolls;
		objs[27] = type.Train;
	}
};