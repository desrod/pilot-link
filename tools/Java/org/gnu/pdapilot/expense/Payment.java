/* 
 * expense/payment.java:
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

public class Payment {
   private int idx;
   final static private String[ ]names = { "AmEx", 
                                           "Cash", 
                                           "Check", 
                                           "Credit Card", 
                                           "MasterCard", 
                                           "Prepaid", 
                                           "VISA", 
                                           "Unfiled" };
   public static Payment amEx = new Payment( 0);
   public static Payment cash =  new Payment( 1);
   public static Payment check =  new Payment( 2);
   public static Payment creditCard =  new Payment( 3);
   public static Payment masterCard =  new Payment( 4);
   public static Payment prepaid =  new Payment( 5);
   public static Payment visa =  new Payment( 6);
   public static Payment unfiled =  new Payment( 7);
   private static Payment[ ]objs;
   private Payment( int value) { this.idx = value; }
   public static Payment get( int value) { return objs[ value]; }
   public static Payment get( String value) {
      int i;
      for( i = 0; i < names.length; i++) {
         if( names[ i].equals( value)) return objs[ i];
         }
      return null;
      }
   public static String[ ]getNames( ) { return names; }
   public String toString( ) { return "Payment." + names[ idx]; }
   public int getValue( ) { return idx; }
   public String getName( ) { return names[ idx]; }
   static {
      objs = new Payment[ 8];
      objs[ 0] = Payment.amEx;
      objs[ 1] = Payment.cash;
      objs[ 2] = Payment.check;
      objs[ 3] = Payment.creditCard;
      objs[ 4] = Payment.masterCard;
      objs[ 5] = Payment.prepaid;
      objs[ 6] = Payment.visa;
      objs[ 7] = Payment.unfiled;
      }
   }
