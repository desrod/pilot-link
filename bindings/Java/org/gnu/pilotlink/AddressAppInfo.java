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
public class AddressAppInfo extends AppInfo {
	private String labels[]; //19+3
	private boolean labelRenamed[];
	private String phoneLabels[]; //whatever
	private int country=0;
	private int sortByCompany=0;
	/**
	 * 
	 */
	public AddressAppInfo() {
		super();
		
	}

	/**
	 * @param ai
	 */
	public AddressAppInfo(AppInfo ai) {
		super(ai);
		
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.AppInfo#setBuffer(byte[])
	 */
	public void setBuffer(byte[] b) {
		labels=new String[22];
		labelRenamed=new boolean[22];
		phoneLabels=new String[8];
		
		buffer=b;
		super.parseCategories();
		long r=Record.getLongAt(buffer,dataOffset);

		for (int i=0; i<22;i++) {
			
			labelRenamed[i]=((r&(1<<i))!=0);
			
		}
		int offset=dataOffset+4;
		for (int i=0;i<22; i++) {
			labels[i]=Record.getStringAt(buffer,offset+i*16);
			//System.out.println("Got label: "+labels[i]);
		}
		offset+=22*16;
		country=Record.getIntAt(buffer,offset);
		offset+=2;
		sortByCompany=buffer[offset];
	
		for (int i=3; i<8; i++) {
			phoneLabels[i-3]=labels[i];
		}
		for (int i=19; i<22; i++) {
			phoneLabels[i-19+5]=labels[i];
		}
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.AppInfo#getBuffer()
	 */
	public byte[] getBuffer() {
		super.createDefaultBuffer();
		long r=0;
		for (int i=0; i<22;i++) {
			if (labelRenamed[i]) {
				r|=(1L<<i);
			}
		}
		int offset=dataOffset+4;
		for (int i=0; i<22;i++) {
			Record.setStringAt(buffer,labels[i],offset+i*16);
		}
		offset+=22*16;
		Record.setIntAt(buffer,country,offset);
		offset+=2;
		buffer[offset]=(byte)sortByCompany;
		
		return buffer;
	}

	/**
	 * @return Returns the country.
	 */
	public int getCountry() {
		return country;
	}

	/**
	 * @param country The country to set.
	 */
	public void setCountry(int country) {
		this.country= country;
	}

	/**
	 * @return Returns the labelRenamed.
	 */
	public boolean getLabelRenamed(int i) {
		return labelRenamed[i];
	}

	/**
	 * @param labelRenamed The labelRenamed to set.
	 */
	public void setLabelRenamed(int i, boolean labelRenamed) {
		this.labelRenamed[i]= labelRenamed;
	}

	/**
	 * @return Returns the labels.
	 */
	public String getLabel(int i) {
		return labels[i];
	}

	/**
	 * @param labels The labels to set.
	 */
	public void setLabel(int i, String labels) {
		this.labels[i]= labels;
	}

	/**
	 * @return Returns the phoneLabels.
	 */
	public String getPhoneLabel(int i) {
		return phoneLabels[i];
	}

	/**
	 * @param phoneLabels The phoneLabels to set.
	 */
	public void setPhoneLabels(int i, String phoneLabel) {
		this.phoneLabels[i]= phoneLabel;
	}

	/**
	 * @return Returns the sortByCompany.
	 */
	public boolean isSortByCompany() {
		return sortByCompany!=0;
	}

	/**
	 * @param sortByCompany The sortByCompany to set.
	 */
	public void setSortByCompany(boolean sortByCompany) {
		if (sortByCompany) {
			this.sortByCompany=1;
		} else  {
			this.sortByCompany=0;
		}
	}

}
