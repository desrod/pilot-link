/*
 * Created on 15.10.2004
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
package org.gnu.pilotlink;

import java.util.Date;

/**
 * @author stephan
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class ToDoRecord extends Record {
	private Date dueDate;
	private byte priority;
	private String description;
	private boolean done;
	private String note;

	public String getDescription() {
		return description;
	}
	public void setDescription(String d) {
		description=d;
		setSize(getBuffer().length);
	}
	public boolean getDone() {
		return done;
	}
	public void setDone(boolean d) {
		done=d;
	}
	public int getPriority() {
		return priority;
	}
	public void setPriority(int i) {
		if (i>255) 
			throw new IllegalArgumentException("Invalid priority");
		priority=(byte)i;
	}
	
	public void setNote(String n) {
		note=n;
		setSize(getBuffer().length);
	}
	public String getNote() {
		return note;
	}
	
	public void setDueDate(Date dd) {
		dueDate=dd;
		setSize(getBuffer().length);
	}
	public Date getDueDate() {
		return dueDate;
	}
	
	public ToDoRecord(Record r) {
		super(r);
	}

	public ToDoRecord() {
	}
	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#getBuffer()
	 */
	public byte[] getBuffer() {
		int size=3;
		if (note!=null) {
			size+=note.length()+1;
		} else {
			size++;
		}
		if (description!=null)  {
			size+=description.length()+1;
		} else {
			size++;
		}
		byte buf[]=new byte[size];
		System.out.println("Size: "+size); 
		if (dueDate!=null) {
			Record.setDateAt(buf,dueDate,0);
		} else {
			buf[0]=(byte)0xff; buf[1]=(byte)0xff;
		}
		buf[2]=priority;
		if(done) {
			buf[2]|=0x80;
		}
		int offset=3;
		if (description!=null) {
			offset=Record.setStringAt(buf,description,offset);			
		} else {
			buf[offset++]=0;
		}
		if (note!=null) {
			Record.setStringAt(buf,note,offset);
		} else {
			buf[offset++]=0;
		}
		return buf;
 	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#setBuffer(byte[])
	 */
	public void setBuffer(byte[] buf) {
		
		if (Record.getIntAt(buf,0) != 0xffff) {
			//due-date set
			System.out.println("\n\nReading in due-date..."+(buf[0]&0xff)+"  "+(buf[1]&0xff)+" getInt "+Record.getIntAt(buf,0)+" "+Record.getDateAt(buf,0));
		
			dueDate=Record.getDateAt(buf,0);
		}
		priority=buf[2];
		if ((priority & 0x80) > 0) {
			//Completed
			priority&=0x7f;
			done=true;
		} else {
			done=false;
		}
		if (buf.length <= 3) {
			return;
		}
		int pos=3;
		description=Record.getStringAt(buf,3);
		if (buf.length<=3+description.length()) {
			return;
		}
		note=Record.getStringAt(buf,3+description.length());
		
		setSize(buf.length);
	}
	
	public String toString() {
		return "P"+priority+": "+description+" Done: "+done+" due to: "+dueDate+" note: "+note; 
	}

}
