/* 
 * Char4.java:
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
import org.gnu.pdapilot.Util;

/**
 * This class is highly suspicious, as it takes no account of code pages
 * and uses deprecated methods.  It needs revision when I have worked
 * out what it is used for.
 */
public class Char4 {
   private int value;
   private byte[ ] b;
   public Char4( int id) {
      this.set( id);
      }
   public Char4( String id) {
      this.set( id);
      }
   public Char4( byte[ ] b) throws java.lang.CloneNotSupportedException {
      this.set( b);
      }
   public Char4( ) {
      this.set( 0);
      }
   public void set( int id) {
      value = id;
      b = new byte[4];
      b[ 3] = ( byte)( id & 0xff); id >>= 8;
      b[ 2] = ( byte)( id & 0xff); id >>= 8;
      b[ 1] = ( byte)( id & 0xff); id >>= 8;
      b[ 0] = ( byte)( id & 0xff);
      }
   public void set( String id) {
      byte[ ] by = id.getBytes( );
      try {
         this.set( by);
         } 
      catch(java.lang.CloneNotSupportedException e) {
         /* Don't be silly! */
         }
      }
   public void set( byte[ ] b) throws java.lang.CloneNotSupportedException{
      this.b = ( byte[ ])b.clone( );
      value = ( b[ 0] << 24) | ( b[ 1] << 16) | ( b[ 2] << 8) | b[ 3];
      }
   public int getInt( ) {
      return value;
      }
   public byte[ ] getBytes( ) {
      return b;
      }
   public String getString( ) {
      return new String( b, 0, b.length);
      }
   public boolean equals( Char4 other) {
      return ( value == other.value);
      }
   public String toString( ) {
      return Util.prettyPrint( b);
      }
   }
