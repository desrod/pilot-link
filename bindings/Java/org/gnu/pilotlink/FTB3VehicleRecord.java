/*
 * Created on 15.02.2004
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
public class FTB3VehicleRecord extends Record {
	private String name;
	private String note;
	private String plate;
	private double monthlyCost;
	/**
	 * 
	 */
	public FTB3VehicleRecord() {
		super();
		
	}

	/**
	 * @param i
	 * @param att
	 * @param cat
	 * @param sz
	 */
	public FTB3VehicleRecord(int i, int att, int cat, int sz) {
		super(i, att, cat, sz);
	
	}

	/**
	 * @param r
	 */
	public FTB3VehicleRecord(Record r) {
		super(r);
		
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#getBuffer()
	 */
	public byte[] getBuffer() {
		byte buf[]=new byte[0x70];
		setStringAt(buf,name,0x08);
		setStringAt(buf,plate,0x18);
		setStringAt(buf,note,0x30);
		return buf;
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#setBuffer(byte[])
	 */
	public void setBuffer(byte[] buf) {	
		name=getStringAt(buf,0x08);
		plate=getStringAt(buf,0x18);
		//TODO: parse monthly cost as float at 0x28
		monthlyCost=getDoubleAt(buf,0x28);
		note=getStringAt(buf,0x30);
		setSize(buf.length);
	}

	/**
	 * @return Returns the name.
	 */
	public String getName() {
		return name;
	}

	/**
	 * @param name The name to set.
	 */
	public void setName(String name) {
		this.name= name;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the note.
	 */
	public String getNote() {
		return note;
	}

	/**
	 * @param note The note to set.
	 */
	public void setNote(String note) {
		this.note= note;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the plate.
	 */
	public String getPlate() {
		return plate;
	}

	/**
	 * @param plate The plate to set.
	 */
	public void setPlate(String plate) {
		this.plate= plate;
		setSize(getBuffer().length);
	}
	
	public String toString() {
		return "Vehicle: "+name+" ("+plate+")  - "+note+ " Cost: "+monthlyCost;
	}

}
