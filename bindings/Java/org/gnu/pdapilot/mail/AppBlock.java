/* 
 * mail/AppBlock.java:
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

package org.gnu.pdapilot.mail;

import java.io.*;

public class AppBlock extends org.gnu.pdapilot.CategoryAppBlock {
   private Sort sortOrder;
   private org.gnu.pdapilot.RecordID unsentMessage;
   private boolean dirty;
   public AppBlock( ) { super( ); }
   public AppBlock( byte[ ]contents) { super( contents); }
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public void fill( ) { }
   public String describe( ) {
      return "sort=" + sortOrder + 
             ", unsentMessage=" + unsentMessage +
             ", dirty=" + dirty + 
             ", " + super.describe( );
      }
   // Accessor Methods
   public Sort getSortOrfer( ) { return sortOrder; }
   public org.gnu.pdapilot.RecordID getUnsentMessage( ) { return unsentMessage; }
   public boolean getDirty( ) { return dirty; }
   public void setSortOrder( Sort s) { sortOrder = s; }
   public void setUnsentMessage( org.gnu.pdapilot.RecordID u) { unsentMessage = u; }
   public void setDirty( boolean d) { dirty = d; }
   }
