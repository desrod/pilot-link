/* 
 * appointment/Record.java:
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

import java.io.*;
import org.gnu.pdapilot.Util;

/** A representation of a datebook database record.
 */

public class Record extends org.gnu.pdapilot.Record {
   private java.util.Date[ ]exceptions;
   private String description;
   private String note;
   private boolean alarm;
   private int advance;
   private Time advanceUnits;
   private Repeat repeatType;
   private java.util.Date repeatEnd;
   private int repeatFrequency;
   private boolean[ ]repeatWeekdays;
   private int repeatDay;
   private int repeatWeekStart;
   private java.util.Date begin, end;
   public Record( ) { super( ); }
   public Record( byte[ ]contents, org.gnu.pdapilot.RecordID id, int index, int attr, int cat) {
      super( contents, id, index, attr, cat);
      }
   protected native void unpack( byte[ ]data);
   protected native byte[ ]pack( );
   public void fill( ) {
      exceptions = null;
      description = "";
      note = null;
      alarm = false;
      advance = 0;
      advanceUnits = Time.minutes;
      repeatType = Repeat.none;
      repeatEnd = null;
      repeatFrequency = 1;
      repeatDay = 0;
      repeatWeekdays = new boolean[7];
      for( int i = 0; i < 7; i++) repeatWeekdays[ i] = false;
      begin = new java.util.Date( );
      end = new java.util.Date( );
      }
   public String describe( ) {
      StringBuffer c = new StringBuffer( "start=");
      c.append( begin.toString( ));
      c.append( ", end=" + Util.prettyPrint( end));
      c.append( ", note=" + ( ( note == null) ? "(null)" : note));
      c.append( ", description=" + ( ( description == null) ? "(null)" : description));
      c.append( ", exceptions=[");
      if( exceptions != null) {
         for( int i = 0; i < exceptions.length; i++) {
            if( i > 0) c.append( ",");
            c.append( exceptions[ i].toString( ));
            }
         }
      c.append( "], advance=" + advance + ", advanceUnits=" + advanceUnits);
      c.append( ", repeatType=" + repeatType + ", repeatEnd=" + ( ( repeatEnd==null) ? "(null)" : repeatEnd.toString( )));
      c.append( ", repeatFrequency=" + repeatFrequency + ", repeatWeekStart=" + repeatWeekStart);
      c.append( ", repeatDay=" + repeatDay + ", repeatWeekdays=[");
      for( int i = 0; i < repeatWeekdays.length; i++) {
         if( i > 0) c.append( ",");
         c.append( repeatWeekdays[ i]);
         }
      c.append( "]");
      return "" + c;
      }
   // Accessor methods
   public java.util.Date[ ] getExceptions( ) { return exceptions; }
   public void setExceptions( java.util.Date[ ]e) { exceptions = e; }
   public String getDescription( ) { return description; }
   public void setDescription( String d) { description = d; }
   public String getNote( ) { return note; }
   public void setNote( String n) { note = n; }
   public boolean getAlarm( ) { return alarm; }
   public void setAlarm( boolean a) { alarm = a; }
   public int getAdvance( ) { return advance; }
   public void setAdvance( int a) { advance = a; }
   public Time getAdvanceUnits( ) { return advanceUnits; }
   public void setAdvanceUnits( Time t) { advanceUnits = t; }
   public Repeat getRepeatType( ) { return repeatType; }
   public void setRepeatType( Repeat r) { repeatType = r; }
   public java.util.Date getRepeatEnd( ) { return repeatEnd; }
   public void setRepeatEnd( java.util.Date d) { repeatEnd = d; }
   public int getRepeatFrequency( ) { return repeatFrequency; }
   public void setRepeatFrequncy( int r) { repeatFrequency = r; }
   public boolean[ ]getRepeatWeekdays( ) { return repeatWeekdays; }
   public void setRepeatWeekdays( boolean[ ]r) { repeatWeekdays = r; }
   public int getRepeatDay( ) { return repeatDay; }
   public void setRepeatDay( int r) { repeatDay = r; }
   public int getRepeatWeekStart( ) { return repeatWeekStart; }
   public void setRepeatWeekStart( int r) { repeatWeekStart = r; }
   public java.util.Date getBegin( ) { return begin; }
   public void setBegin( java.util.Date b) { begin = b; }
   public java.util.Date getEnd( ) { return end; }
   public void setEnd( java.util.Date e) { end = e; }
   }
