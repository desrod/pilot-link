/* 
 * appointment/repeat.java:
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

package org.gnu.pdapilot.appointment;

public class Repeat {
   private int idx;
   final static private String[ ]names = 
      { "none", "daily", "weekly", "monthlyByDay", "monthlyByDate", "yearly" };
   public static Repeat none = new Repeat( 0);
   public static Repeat daily = new Repeat( 1);
   public static Repeat weekly = new Repeat( 2);
   public static Repeat monthlyByDay = new Repeat( 3);
   public static Repeat monthlyByDate = new Repeat( 4);
   public static Repeat yearly = new Repeat( 5);
   private static Repeat[ ]objs;
   private Repeat( int value) {
      this.idx = value;
      }
   public static Repeat get( int value) {
      return objs[ value];
      }
   public static Repeat get( String value) {
      int i;
      for( i = 0; i < names.length; i++) {
         if( names[ i].equals( value)) return objs[ i];
         }
      return null;
      }
   public static String[ ]getNames( ) { return names; }
   public String toString( ) { return "Repeat." + names[ idx]; }
   public int getValue( ) { return idx; }
   public String getName( ) { return names[ idx]; }
   static {
      objs = new Repeat[ 6];
      objs[ 0] = Repeat.none;
      objs[ 1] = Repeat.daily;
      objs[ 2] = Repeat.weekly;
      objs[ 3] = Repeat.monthlyByDay;
      objs[ 4] = Repeat.monthlyByDate;
      objs[ 5] = Repeat.yearly; 
      }
   }
