/* 
 * mail/Record.java:
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

/** A representation of a mail database record.
 */

public class Record extends org.gnu.pdapilot.Record {
   private boolean read, signature, confirmRead, confirmDelivery;
   private int priority, addressing;
   private java.util.Date date;
   private String subject, from, to, cc, bcc, replyTo, sentTo, body;
   public Record( ) { super( ); }
   public Record( byte[ ]contents, org.gnu.pdapilot.RecordID id, int index, int attr, int cat) {
      super( contents, id, index, attr, cat);
      }
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public void fill( ) { }
   public String describe( ) {
      StringBuffer c = new StringBuffer( "read=");
      return "read=" + read + 
             ", signature=" + signature + 
             ", confirmRead=" + confirmRead +
             ", confirmDelivery=" + confirmDelivery + 
             ", priority=" + priority +
             ", addressing=" + addressing + 
             ", date=" + ( ( date == null) ? "<Null>" : date.toString( ))+
             ", from=" + Util.prettyPrint( from) +
             ", to=" + Util.prettyPrint( to) +
             ", cc=" + Util.prettyPrint( cc) +
             ", bcc=" + Util.prettyPrint( bcc) +
             ", replyTo=" + Util.prettyPrint( replyTo) +
             ", sentTo=" + Util.prettyPrint( sentTo) +
             ", body=" + Util.prettyPrint( body) +
             ", subject=" + Util.prettyPrint( subject) +
             ", " + super.describe( );
      }
   // Accessor Methods 
   public boolean getRead( ) { return read; }
   public boolean getSignature( ) { return signature; }
   public boolean getConfirmRead( ) { return confirmRead; }
   public boolean getConfirmDelivery( ) { return confirmDelivery; }
   public int getPriority( ) { return priority; }
   public int getAddressing( ) { return addressing; }
   public java.util.Date getDate( ) { return date; }
   public String getSubject( ) { return subject; }
   public String getFrom( ) { return from; }
   public String getTo( ) { return to; }
   public String getCc( ) { return cc; }
   public String getBcc( ) { return bcc; }
   public String getReplyTo( ) { return replyTo; }
   public String getSentTo( ) { return sentTo; }
   public String getBody( ) { return body; }
   public void setRead( boolean r) { read = r; }
   public void setSignature( boolean s) { signature = s; }
   public void setConfirmRead( boolean r) { confirmRead = r; }
   public void setConfirmDelivery( boolean d) { confirmDelivery = d; }
   public void setPriority( int p) { priority = p; }
   public void setAddressing( int a) { addressing = a; }
   public void setDate( java.util.Date d) { date = d; }
   public void setSubject( String s) { subject = s; }
   public void setFrom( String f) { from = f; }
   public void setTo( String t) { to = t; }
   public void setCc( String c) { cc = c; }
   public void setBcc( String b) { bcc = b; }
   public void setReplyTo( String r) { replyTo = r; }
   public void setSentTo( String s) { sentTo = s; }
   public void setBody( String b) { body = b; }
   }
