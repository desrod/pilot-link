/* 
 * end.java:
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

package org.gnu.pdapilot;

public class End {
   private int idx;
   static private String[ ] names = null;
   static private End[ ] objs = null;
   public static End normal = new End( 0, "Normal");
   public static End outOfMemory =  new End( 1, "OutOfMemory");
   public static End userCancelled =  new End( 2, "UserCancelled");
   public static End other =  new End( 3, "Other");
   private End( int value, String name) {
      if( objs == null) objs = new End[ 4];
      if( names == null) names = new String[ 4];
      this.idx = value;
      objs[ value] = this;
      names[ value] = name;
      }
   public static End get( int value) {
      return objs[ value];
      }
   public static End get(String value) {
      int i;
      for( i=0; i < names.length; i++) {
         if( names[ i].equals( value)) return objs[ i];
         }
      return null;
      }
   public static String[ ]getNames( ) {
      return names;
      }
   public String toString( ) {
      return "End." + names[ idx];
      }
   public int getValue( ) {
      return idx;
      }
   public String getName( ) {
      return names[ idx];
      }
   }
