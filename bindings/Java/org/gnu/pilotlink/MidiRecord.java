/*
 * Created on 16.03.2004
 *
 * To change the template for this generated file go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
package org.gnu.pilotlink;


/**
 * @author stephan
 *
 * To change the template for this generated type comment go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
public class MidiRecord extends Record {
	private byte midi[];
	private String name;

	private static String id="PMrc";
	
	public MidiRecord() {
		midi=new byte[1]; //avoid nullpointer exception
		name="";
	}
	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#getBuffer()
	 */
	public byte[] getBuffer() {
		// TODO Auto-generated method stub
		int offset=name.length()+1+6;
		byte buf[]=new byte[offset+midi.length];
		setStringAt(buf,id,0);
		buf[4]=(byte)offset;
		buf[5]=0;
		setStringAt(buf,name,6);
		for (int i=0; i<midi.length;i++) {
			buf[offset+i]=midi[i];
		}
		return buf;
	}
	
	public void setName(String n) {
		name=n;
		setSize(midi.length+2+id.length()+name.length()+1);
	}
	public String getName() {
		
		return name;
	}
	public void setMidi(byte b[]) {
		midi=b;
		setSize(midi.length+2+id.length()+name.length()+1);
	}
	public byte[] getMidi() {
		return midi;
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#setBuffer(byte[])
	 */
	public void setBuffer(byte[] buf) {
		// TODO Auto-generated method stub
		name=getStringAt(buf,6);
		int offset=buf[4];
		midi=new byte[buf.length-offset];
		for (int i=0;i<midi.length;i++) {
			midi[i]=buf[i+offset];
		}
		setSize(buf.length);
	}

	/**
	 * @param r
	 */
	public MidiRecord(Record r) {
		super(r);
	}

	
}
