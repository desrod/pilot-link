/* 
 * expense/CustomCurrency.java:
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

public class CustomCurrency {
   private String name, symbol, rate;
   public String toString( ) {
      return "<org.gnu.pdapilot.expense.CustomCurrency name=" + name + 
             ", symbol=" + symbol +
             ", rate=" + rate + ">";
      }
   // Accessor Methods
   public String getName( ) { return name; }
   public String getSymbol( ) { return symbol; }
   public String getRate( ) { return rate; }
   public void setName( String n) { name = n; }
   public void setSymbol( String s) { symbol = s; }
   public void setRate( String r) { rate = r; }
   }
