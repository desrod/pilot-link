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
public class FTB3CostCatRecord extends Record {
	private String name;
	/**
	 * 
	 */
	public FTB3CostCatRecord() {
		super();
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param i
	 * @param att
	 * @param cat
	 * @param sz
	 */
	public FTB3CostCatRecord(int i, int att, int cat, int sz) {
		super(i, att, cat, sz);
		// TODO Auto-generated constructor stub
	}

	/**
	 * @param r
	 */
	public FTB3CostCatRecord(Record r) {
		super(r);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#getBuffer()
	 */
	public byte[] getBuffer() {
		byte buf[]=new byte[0x48];
		if (name==null) {
			return buf;
		}
		if (name.length()>0x40) {
			name=name.substring(0,0x40);
		}
		setStringAt(buf,name,0x08);
		return buf;
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#setBuffer(byte[])
	 */
	public void setBuffer(byte[] buf) {

		name=getStringAt(buf,0x08);

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
	}

	public String toString(){
		return ""+name;
	}
}
