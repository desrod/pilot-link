/* 
 * mail/Sync.java:
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

public class Sync {
   private int idx;
   final static private String[ ]names = 
      { "All", "Send", "Filter" };
   public static Sync all = new Sync( 0);
   public static Sync send =  new Sync( 1);
   public static Sync filter = new Sync( 2);
   private static Sync[ ]objs;
   private Sync( int value) { this.idx = value; }
   public static Sync get( int value) { return objs[ value]; }
   public static Sync get( String value) {
      int i;
      for( i = 0; i < names.length; i++) {
         if( names[ i].equals( value)) return objs[ i];
         }
      return null;
      }
   public static String[ ] getNames( ) { return names; }
   public String toString( ) { return "Sync." + names[ idx]; }
   public int getValue( ) { return idx; }
   public String getName( ) { return names[ idx]; }
   static {
      objs = new Sync[ 3];
      objs[ 0] = Sync.all;
      objs[ 1] = Sync.send;
      objs[ 2] = Sync.filter;
      }
   }
