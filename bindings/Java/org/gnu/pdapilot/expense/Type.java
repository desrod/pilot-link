/* 
 * expense/Type.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 * Copyright (C) 2001 David.Goodenough@DGA.co.uk
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

package org.gnu.pdapilot.expense;

public class Type {
   private int idx;
   final static private String[ ]names = { 
      "Airfare", "Breakfast", "Bus", "Business Meals", "Car Rental", 
      "Dinner", "Entertainment", "Fax", "Gas", "Gifts", "Hotel",
      "Incidentals", "Laundry", "Limo", "Lodging", "Lunch", "Mileage",
      "Other", "Parking", "Postage", "Snack", "Subway", "Supplies",
      "Taxi", "Telephone", "Tips", "Tolls", "Train" };
   public static Type airfare = new Type( 0);
   public static Type breakfast =  new Type( 1);
   public static Type bus =  new Type( 2);
   public static Type businessMeals =  new Type( 3);
   public static Type carRental =  new Type( 4);
   public static Type dinner =  new Type( 5);
   public static Type entertainment =  new Type( 6);
   public static Type fax =  new Type( 7);
   public static Type gas =  new Type( 8);
   public static Type gifts =  new Type( 9);
   public static Type hotel =  new Type( 10);
   public static Type incidentals =  new Type( 11);
   public static Type laundry =  new Type( 12);
   public static Type limo =  new Type( 13);
   public static Type lodging =  new Type( 14);
   public static Type lunch =  new Type( 15);
   public static Type mileage =  new Type( 16);
   public static Type other =  new Type( 17);
   public static Type parking =  new Type( 18);
   public static Type postage =  new Type( 19);
   public static Type snack =  new Type( 20);
   public static Type subway =  new Type( 21);
   public static Type supplies =  new Type( 22);
   public static Type taxi =  new Type( 23);
   public static Type telephone =  new Type( 24);
   public static Type tips =  new Type( 25);
   public static Type tolls =  new Type( 26);
   public static Type train =  new Type( 27);
   private static Type[ ]objs;
   private Type( int value) { this.idx = value; }
   public static Type get( int value) { return objs[ value]; }
   public static Type get( String value) {
      int i;
      for( i = 0; i < names.length; i++) {
         if( names[ i].equals( value)) return objs[ i];
         }
      return null;
      }
   public static String[ ]getNames( ) { return names; }
   public String toString( ) { return "Type." + names[ idx]; }
   public int getValue( ) { return idx; }
   public String getName( ) { return names[ idx]; }
   static {
      objs = new Type[28];
      objs[ 0] = Type.airfare;
      objs[ 1] = Type.breakfast;
      objs[ 2] = Type.bus;
      objs[ 3] = Type.businessMeals;
      objs[ 4] = Type.carRental;
      objs[ 5] = Type.dinner;
      objs[ 6] = Type.entertainment;
      objs[ 7] = Type.fax;
      objs[ 8] = Type.gas;
      objs[ 9] = Type.gifts;
      objs[ 10] = Type.hotel;
      objs[ 11] = Type.incidentals;
      objs[ 12] = Type.laundry;
      objs[ 13] = Type.limo;
      objs[ 14] = Type.lodging;
      objs[ 15] = Type.lunch;
      objs[ 16] = Type.mileage;
      objs[ 17] = Type.other;
      objs[ 18] = Type.parking;
      objs[ 19] = Type.postage;
      objs[ 20] = Type.snack;
      objs[ 21] = Type.subway;
      objs[ 22] = Type.supplies;
      objs[ 23] = Type.taxi;
      objs[ 24] = Type.telephone;
      objs[ 25] = Type.tips;
      objs[ 26] = Type.tolls;
      objs[ 27] = Type.train;
      }
   }
