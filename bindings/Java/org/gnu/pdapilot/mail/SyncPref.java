/* 
 * mail/SyncPref.java:
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
import org.gnu.pdapilot.Util;

public class SyncPref extends org.gnu.pdapilot.Pref {
   private Sync syncType;
   private boolean getHigh;
   private boolean getContaining, truncate;
   private String filterTo, filterFrom, filterSubject;
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public SyncPref( ) {
      super( null, new org.gnu.pdapilot.Char4( "mail"), 1, 1, true);
      }
   public SyncPref( boolean local) {
      super( null, new org.gnu.pdapilot.Char4( "mail"), local ? 1 : 2, 1, true);
      }
   public SyncPref( byte[ ]contents, org.gnu.pdapilot.Char4 creator, int id, int version, boolean backup) {
      super( contents, creator, id, version, backup);
      }
   public String describe( ) {
      return "syncType=" + syncType + 
             ", getHigh=" + getHigh + 
             ", getContaining=" + getContaining + 
             ", truncate=" + truncate + 
             ", filterTo=" + Util.prettyPrint( filterTo) +
             ", filterFrom=" + Util.prettyPrint( filterFrom) +
             ", filterSubject=" + Util.prettyPrint( filterSubject) + 
             ", " + super.describe( );
      }
   // Accessor Methods
   public Sync getSyncType( ) { return syncType; } 
   public boolean getGetHigh( ) { return getHigh; }
   public boolean getGetContaining( ) { return getContaining; }
   public boolean getTruncate( ) { return truncate; }
   public String getFilterTo( ) { return filterTo; }
   public String getFilterFrom( ) { return filterFrom; }
   public String getFilterSubject( ) { return filterSubject; }
   public void setSyncType( Sync s) { syncType = s; }
   public void setGetHigh( boolean g) { getHigh = g; }
   public void setGetContaining( boolean g) { getContaining = g; }
   public void setTruncate( boolean t) { truncate = t; }
   public void setFilterTo( String f) { filterTo = f; }
   public void setFilterFrom( String f) { filterFrom = f; }
   public void setFilterSubject( String f) { filterSubject = f; }
   }
