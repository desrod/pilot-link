/* 
 * 1.1/Util.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
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

public class Util {
   static public String prettyPrint( byte[ ]b) { 
      return prettyPrint( new String( b, 0, b.length));  
      }
   static public String prettyPrint( Object o) {
      if( o == null) return "null";
      char[ ] c = o.toString( ).toCharArray( );
      StringBuffer out = new StringBuffer( );
      for( int i = 0; i < c.length; i++) {
         if( c[ i] == 13)         out.append( "\\r");
            else if( c[ i] == 10) out.append( "\\n");
            else if( c[ i] == 8)  out.append( "\\b");
            else if( c[ i] == 9)  out.append( "\\t");
            else if( c[ i] < 32) {
               out.append( '^');
               out.append( ( char)( c[ i] ^ 64));
               }
            else if( c[ i] > 126) {
               out.append( "\\x");
               out.append( Integer.toHexString( c[ i]));
               } 
            else out.append( c[ i]);
         }
      return out.toString();
      }
   static public String readLine( ) throws java.io.IOException {
      java.io.BufferedReader dis = new java.io.BufferedReader( new java.io.InputStreamReader( System.in));
      return dis.readLine( );
      }
   }
