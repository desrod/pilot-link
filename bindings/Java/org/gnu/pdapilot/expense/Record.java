/* 
 * expense/Record.java:
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

/** A representation of an expense database record.
 */

public class Record extends org.gnu.pdapilot.Record {
   private java.util.Date date;
   private String amount, vendor, city, attendees, note;
   private int currency;
   private Type type;
   private Payment payment;
   public Record( ) { super( ); }
   public Record( byte[ ]contents, org.gnu.pdapilot.RecordID id, int index, int attr, int cat) {
      super( contents, id, index, attr, cat);
      }
   protected native void unpack( byte[ ]data);
   protected native byte[ ] pack( );
   public void fill( ) {
      type = Type.telephone;
      payment = Payment.cash;
      }
   public String describe( ) {
      return "date=" + Util.prettyPrint( date) +
             ", currency=" + currency +
             ", type=" + type + 
             ", payment=" + payment +
             ", amount=" + Util.prettyPrint( amount) +
             ", vendor=" + Util.prettyPrint( vendor) +
             ", city=" + Util.prettyPrint( city) +
             ", attendees=" + Util.prettyPrint( attendees) +
             ", note=" + Util.prettyPrint( note) +
             ", " + super.describe( );
      }
   // Accessor Methods
   public java.util.Date getDate( ) { return date; }
   public String getAmount( ) { return amount; }
   public String getVendor( ) { return vendor; } 
   public String getCity( ) { return city; }
   public String getAttendees( ) { return attendees; } 
   public String getNote( ) { return note; }
   public int getCurrency( ) { return currency; } 
   public Type getType( ) { return type; }
   public Payment getPayment( ) { return payment; } 
   public void setDate( java.util.Date d) { date = d; }
   public void setAmount( String a) { amount = a; }
   public void setVendor( String v) { vendor = v; } 
   public void setCity( String c) { city = c; }
   public void setAttendees( String a) { attendees = a; }
   public void setNote( String n) { note = n; }
   public void setCurrency( int c) { currency = c; }
   public void setType( Type t) { type = t; }
   public void setPayment( Payment p) { payment = p; }
   }
