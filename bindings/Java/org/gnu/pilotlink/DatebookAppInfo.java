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
public class DatebookAppInfo extends AppInfo {
	private int startOfWeek=0;
	
	public DatebookAppInfo(int startofweek) {
		startOfWeek=startofweek;
	}
	/**
	 * @param ai
	 */
	public DatebookAppInfo(AppInfo ai) {
		super(ai);
		// TODO Auto-generated constructor stub
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.AppInfo#setBuffer(byte[])
	 */
	public void setBuffer(byte[] b) {
		parseCategories();
		startOfWeek=b[dataOffset];
	}

	/**
	 * @return Returns the startOfWeek.
	 */
	public int getStartOfWeek() {
		return startOfWeek;
	}

	/**
	 * @param startOfWeek The startOfWeek to set.
	 */
	public void setStartOfWeek(int startOfWeek) {
		this.startOfWeek= startOfWeek;
	}
	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.AppInfo#getBuffer()
	 */
	public byte[] getBuffer() {
		createDefaultBuffer();
		buffer[dataOffset]=(byte)startOfWeek;
		return buffer;
	}

}
