/* 
 * address/Record.java:
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

package org.gnu.pdapilot.address;

import java.io.*;

/** A representation of an address database record.
 */

public class Record extends org.gnu.pdapilot.Record {
   private int[ ]phoneLabel;
   private int showPhone;
   private String[ ]entry;
   public Record( ) { super( ); }
   public Record( byte[ ]contents, org.gnu.pdapilot.RecordID id, int index, int attr, int cat) {
      super( contents, id, index, attr, cat);
      }
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public void fill( ) {
      entry = new String[ 19];
      phoneLabel = new int[ 5];
      showPhone = 0;
      }
		
   public String describe( ) {
      StringBuffer c = new StringBuffer( "phoneLabel=[");
      for( int i = 0; i < phoneLabel.length; i++) {
         if( i>0) c.append( ",");
         c.append( phoneLabel[ i]);
         }
      c.append( "], entry=[");
      for( int i = 0; i < entry.length; i++) {
         if( i > 0) c.append( ",");
         c.append( ( entry[ i] == null) ? "(null)" : entry[ i]);
         }
      c.append( "]");
      return "showPhone=" + showPhone + ", " + c + ", " + super.describe( );
      }
   // Accessor methods
   public int[ ]getPhoneLabel( ) { return phoneLabel; }
   public int getShowPhone( ) { return showPhone; }
   public String[ ]getEntry( ) { return entry; } 
   public void setPhoneLabel( int[ ]p) { phoneLabel = p; }
   public void setShowPhone( int s) { showPhone = s; }
   public void setEntry( String[ ]e) { entry = e; }
   }
