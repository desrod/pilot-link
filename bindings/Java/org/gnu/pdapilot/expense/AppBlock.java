/* 
 * expense/AppBlock.java:
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

import java.io.*;
import org.gnu.pdapilot.Util;

public class AppBlock extends org.gnu.pdapilot.CategoryAppBlock {
   private Sort sortOrder;
   private CustomCurrency[ ]currencies;
   public AppBlock( ) {
      super( );
      }
   public AppBlock( byte[ ]contents) {
      super( contents);
      }
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public void fill( ) {
      currencies = new CustomCurrency[ 4];
      }
   public String describe( ) {
      StringBuffer c = new StringBuffer( );
      c.append( "currencies=[");
      if( currencies != null) {
         for( int i = 0; i < currencies.length; i++) {
            if( i > 0) c.append( ",");
            c.append( "[");
            c.append( "name=" + Util.prettyPrint( currencies[ i].getName( )));
            c.append( ",symbol=" + Util.prettyPrint( currencies[ i].getSymbol( )));
            c.append( ",rate=" + Util.prettyPrint( currencies[ i].getRate( )));
            c.append( "]");
            }
         }
      c.append( "]");
      return "sort=" + sortOrder + ", " + c + ", " + super.describe( );
      }
   // Accessor methods
   public Sort getSortOrder( ) { return sortOrder; }
   public CustomCurrency[ ]getCurrencies( ) { return currencies; }
   public void setSortOrder( Sort s) { sortOrder = s; }
   public void setCurrencies( CustomCurrency[ ]c) { currencies = c; }
   }
