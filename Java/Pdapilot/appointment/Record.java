/* 
 * appointment/Record.java:
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

package Pdapilot.appointment;

import java.io.*;

/** A representation of a datebook database record.
 */

public class Record extends Pdapilot.Record {

	public java.util.Date[] exceptions;
	public String description;
	public String note;
	public boolean alarm;
	public int advance;
	public time advanceUnits;
	public repeat repeatType;
	public java.util.Date repeatEnd;
	public int repeatFrequency;
	public boolean[] repeatWeekdays;
	public int repeatDay;
	public int repeatWeekStart;
	public java.util.Date begin, end;

		public Record() {
			super();
		}
		
		public Record(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) {
			super(contents, id, index, attr, cat);
		}
		
		public native void unpack(byte[] data);
		public native byte[] pack();
        		
		public void fill() {
			exceptions = null;
			description = "";
			note = null;
			alarm = false;
			advance = 0;
			advanceUnits = time.Minutes;
			repeatType = repeat.None;
			repeatEnd = null;
			repeatFrequency = 1;
			repeatDay = 0;
			repeatWeekdays = new boolean[7];
			for (int i=0;i<7;i++)
				repeatWeekdays[i] = false;
			begin = new java.util.Date();
			end = new java.util.Date();
		}
		
        public String describe() {
        	StringBuffer c = new StringBuffer("start=");
        	c.append(begin.toString());
        	c.append(", end="+Pdapilot.Util.prettyPrint(end));
        	c.append(", note="+((note == null) ? "(null)" : note));
        	c.append(", description="+((description == null) ? "(null)" : description));
        	c.append(", exceptions=[");
        	if (exceptions != null) {
	        	for(int i=0;i<exceptions.length;i++) {
	        		if (i>0)
	        			c.append(",");
	        		c.append(exceptions[i].toString());
	        	}
	        }
        	c.append("], advance="+advance+", advanceUnits="+advanceUnits);
        	c.append(", repeatType="+repeatType+", repeatEnd="+((repeatEnd==null)?"(null)":repeatEnd.toString()));
        	c.append(", repeatFrequency="+repeatFrequency+", repeatWeekStart="+repeatWeekStart);
        	c.append(", repeatDay="+repeatDay+", repeatWeekdays=[");
        	for(int i=0;i<repeatWeekdays.length;i++) {
        		if (i>0)
        			c.append(",");
        		c.append(repeatWeekdays[i]);
        	}
        	c.append("]");
        	return "" + c;
        }
}
