package org.gnu.pilotlink;
import java.util.Date;
import java.util.GregorianCalendar;
/**
 *  Description of the Class
 *
 *@author     stephan
 *@created    5. Dezember 2004
 */
public abstract class Record {
	/**
	 *  Description of the Field
	 */
	public final static int DELETED = 0x80;
	/**
	 *  Description of the Field
	 */
	public final static int DIRTY = 0x40;
	/**
	 *  Description of the Field
	 */
	public final static int BUSY = 0x20;
	/**
	 *  Description of the Field
	 */
	public final static int SECRET = 0x10;
	/**
	 *  Description of the Field
	 */
	public final static int ARCHIVED = 0x8;

	/**
	 *  Description of the Field
	 */
	protected long id;
	/**
	 *  Description of the Field
	 */
	protected int attribs;
	/**
	 *  Description of the Field
	 */
	protected int category;
	/**
	 *  Description of the Field
	 */
	protected int size;


	/**
	 *  Constructor for the Record object
	 */
	public Record() {
		id = 0;
		attribs = 0;
		category = 0;
		size = 0;
	}


	/**
	 *  Gets the deleted attribute of the Record object
	 *
	 *@return    The deleted value
	 */
	public boolean isDeleted() {
		return (attribs & DELETED) != 0;
	}


	/**
	 *  Gets the secret attribute of the Record object
	 *
	 *@return    The secret value
	 */
	public boolean isSecret() {
		return (attribs & SECRET) != 0;
	}


	/**
	 *  Gets the archived attribute of the Record object
	 *
	 *@return    The archived value
	 */
	public boolean isArchived() {
		return (attribs & ARCHIVED) != 0;
	}


	/**
	 *  Gets the dirty attribute of the Record object
	 *
	 *@return    The dirty value
	 */
	public boolean isDirty() {
		return (attribs & DIRTY) != 0;
	}


	/**
	 *  Gets the busy attribute of the Record object
	 *
	 *@return    The busy value
	 */
	public boolean isBusy() {
		return (attribs & BUSY) != 0;
	}


	/**
	 *  Sets the dirty attribute of the Record object
	 *
	 *@param  d  The new dirty value
	 */
	public void setDirty(boolean d) {
		if (d) {
			attribs = attribs | DIRTY;
		} else {
			attribs = attribs & (~DIRTY);
		}
	}
	
	public void setSecret(boolean b) {
		if (b) {
			attribs |=SECRET;
		} else {
			attribs &=(~SECRET);
		}
	}


	/**
	 *  Sets the archived attribute of the Record object
	 *
	 *@param  a  The new archived value
	 */
	public void setArchived(boolean a) {
		if (a) {
			attribs = attribs | ARCHIVED;
		} else {
			attribs = attribs & (~ARCHIVED);
		}
	}


	/**
	 *  Constructor for the Record object
	 *
	 *@param  i    Description of the Parameter
	 *@param  att  Description of the Parameter
	 *@param  cat  Description of the Parameter
	 *@param  sz   Description of the Parameter
	 */
	public Record(int i, int att, int cat, int sz) {
		size = sz;
		id = i;
		attribs = att;
		category = cat;
	}
 

	/**
	 *  Constructor for the Record object
	 *
	 *@param  r  Description of the Parameter
	 */
	public Record(Record r) {
		size = r.getSize();
		id = r.getId();
		attribs = r.getAttribs();
		category = r.getCategory();
		setBuffer(r.getBuffer());
	}


	/**
	 *  Sets the id attribute of the Record object
	 *
	 *@param  i  The new id value
	 */
	public void setId(long i) {
		id = i;
	}


	/**
	 *  Gets the category attribute of the Record object
	 *
	 *@return    The category value
	 */
	public int getCategory() {
		return category;
	}


	/**
	 *  Sets the category attribute of the Record object
	 *
	 *@param  c  The new category value
	 */
	public void setCategory(int c) {
		category = c;
	}


	/**
	 *  Sets the attribs attribute of the Record object
	 *
	 *@param  a  The new attribs value
	 */
	public void setAttribs(int a) {
		attribs = a;
	}


	/**
	 *  Gets the attribs attribute of the Record object
	 *
	 *@return    The attribs value
	 */
	public int getAttribs() {
		return attribs;
	}


	/**
	 *  Gets the id attribute of the Record object
	 *
	 *@return    The id value
	 */
	public long getId() {
		return id;
	}


	/**
	 *  Gets the size attribute of the Record object
	 *
	 *@return    The size value
	 */
	public int getSize() {
		return size;
	}


	/**
	 *  Sets the size attribute of the Record object
	 *
	 *@param  s  The new size value
	 */
	public void setSize(int s) {
		size = s;
	}


	/**
	 *  Gets the buffer attribute of the Record object
	 *
	 *@return    The buffer value
	 */
	public abstract byte[] getBuffer();


	/**
	 *  Sets the buffer attribute of the Record object
	 *
	 *@param  buf  The new buffer value
	 */
	public abstract void setBuffer(byte buf[]);


	/**
	 *  Gets the dateAt attribute of the Record object
	 *
	 *@param  idx  Description of the Parameter
	 *@return      The dateAt value
	 */
	public Date getDateAt(int idx) {
		return getDateAt(getBuffer(), idx);
	}


	/**
	 *  Gets the dateAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The dateAt value
	 */
	public static Date getDateAt(byte buffer[], int idx) {
		int binary = (buffer[idx] & 0xff) * 256 + (buffer[idx + 1] & 0xff);
		int year = binary & 0xfe00;
		year = year >> 9;
		year += 1904;
		int month = binary & 0x01e0;
		month = month >> 5;
		month--;
		int day = binary & 0x001f;
		//System.out.println("Y: "+year+" M:"+month+" D: "+day);
		GregorianCalendar cal = new GregorianCalendar(year, month, day);
		return cal.getTime();
	}


	/**
	 *  Sets the dateAt attribute of the Record class
	 *
	 *@param  buffer  The new dateAt value
	 *@param  d       The new dateAt value
	 *@param  idx     The new dateAt value
	 */
	public static void setDateAt(byte buffer[], Date d, int idx) {
		GregorianCalendar cal = new GregorianCalendar();
		//System.out.println("Setting date "+d+" at: "+idx);
		cal.setTime(d);
		int binary = 0;
		int year = cal.get(GregorianCalendar.YEAR);
		int month = cal.get(GregorianCalendar.MONTH);
		int day = cal.get(GregorianCalendar.DAY_OF_MONTH);
		year -= 1904;
		year = year << 9;
		month++;
		month = month << 5;

		binary = year | month | day;

		buffer[idx + 1] = (byte) (binary & 0x00ff);
		buffer[idx] = (byte) ((binary & 0xff00) >> 8);
	}


	/**
	 *  Sets the dateTimeAt attribute of the Record class
	 *
	 *@param  buffer  The new dateTimeAt value
	 *@param  d       The new dateTimeAt value
	 *@param  idx     The new dateTimeAt value
	 *@param  tidx    The new dateTimeAt value
	 */
	public static void setDateTimeAt(byte buffer[], Date d, int idx, int tidx) {
		setDateAt(buffer, d, idx);
		GregorianCalendar cal = new GregorianCalendar();
		cal.setTime(d);
		int h = cal.get(GregorianCalendar.HOUR_OF_DAY);
		int m = cal.get(GregorianCalendar.MINUTE);
		buffer[tidx] = (byte) h;
		buffer[tidx + 1] = (byte) m;
	}


	/**
	 *  Gets the dateTimeAt attribute of the Record object
	 *
	 *@param  idx   Description of the Parameter
	 *@param  tidx  Description of the Parameter
	 *@return       The dateTimeAt value
	 */
	public Date getDateTimeAt(int idx, int tidx) {
		return getDateTimeAt(getBuffer(), idx, tidx);
	}


	/**
	 *  Gets the dateTimeAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@param  tidx    Description of the Parameter
	 *@return         The dateTimeAt value
	 */
	public static Date getDateTimeAt(byte buffer[], int idx, int tidx) {
		int binary = (buffer[idx] & 0xff) * 256 + (buffer[idx + 1] & 0xff);
		int year = binary & 0xfe00;
		year = year >> 9;
		year += 1904;
		int month = binary & 0x01e0;
		month = month >> 5;
		month--;
		int day = binary & 0x001f;
		//System.out.println("Y: "+year+" M:"+month+" D: "+day);
		int hour = buffer[tidx] & 0xff;
		int min = buffer[tidx + 1] & 0xff;
		GregorianCalendar cal = new GregorianCalendar(year, month, day, hour, min, 0);
		return cal.getTime();
	}


	/**
	 *  Sets the stringAt attribute of the Record class
	 *
	 *@param  buffer  The new stringAt value
	 *@param  s       The new stringAt value
	 *@param  idx     The new stringAt value
	 *@return         Description of the Return Value
	 */
	public static int setStringAt(byte buffer[], String s, int idx) {
		if (s == null) {
			return idx;
		}

		byte str[];

		str = s.getBytes();

		for (int i = 0; i < str.length; i++) {
			if (s.charAt(i) == 'ü') {
				buffer[idx + i] = (byte) 252;
			} else if (s.charAt(i) == 'ö') {
				buffer[idx + i] = (byte) 246;
			} else if (s.charAt(i) == 'ä') {
				buffer[idx + i] = (byte) 228;
			} else if (s.charAt(i) == 'ß') {
				buffer[idx + i] = (byte) 223;
			} else if (s.charAt(i) == 'Ä') {
				buffer[idx + i] = (byte) 196;
			} else if (s.charAt(i) == 'Ü') {
				buffer[idx + i] = (byte) 220;
			} else if (s.charAt(i) == 'Ö') {
				buffer[idx + i] = (byte) 214;
			} else {
				buffer[idx + i] = str[i];
			}
		}
		//System.out.println();
		buffer[idx + str.length] = 0;
		return (idx + str.length + 1);
	}


	/**
	 *  Gets the stringAt attribute of the Record object
	 *
	 *@param  idx  Description of the Parameter
	 *@return      The stringAt value
	 */
	public String getStringAt(int idx) {
		return getStringAt(getBuffer(), idx);
	}


	/**
	 *  Gets the stringAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The stringAt value
	 */
	public static String getStringAt(byte buffer[], int idx) {
		String str = "";
		while (idx < buffer.length && buffer[idx] != 0) {
			if (buffer[idx] + 256 == 252) {
				str += "ü";
			} else if (buffer[idx] + 256 == 246) {
				str += "ö";
			} else if (buffer[idx] + 256 == 228) {
				str += "ä";
			} else if (buffer[idx] + 256 == 223) {
				str += "ß";
			} else if (buffer[idx] + 256 == 196) {
				str += "Ä";
			} else if (buffer[idx] + 256 == 220) {
				str += "Ü";
			} else if (buffer[idx] + 256 == 214) {
				str += "Ö";
			} else {
				str += (char) buffer[idx];
			}
			idx++;
		}

		return str;
	}


	/**
	 *  Sets the intAt attribute of the Record class
	 *
	 *@param  buffer  The new intAt value
	 *@param  i       The new intAt value
	 *@param  idx     The new intAt value
	 */
	public static void setIntAt(byte buffer[], int i, int idx) {
		buffer[idx + 1] = (byte) (i & 0x00ff);
		buffer[idx] = (byte) ((i & 0xff00) >> 8);
	}


	/**
	 *  Gets the intAt attribute of the Record object
	 *
	 *@param  idx  Description of the Parameter
	 *@return      The intAt value
	 */
	public int getIntAt(int idx) {
		return getIntAt(getBuffer(), idx);
	}


	/**
	 *  Gets the intAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The intAt value
	 */
	public static int getIntAt(byte buffer[], int idx) {

		return ((buffer[idx] & 0xff) << 8) + (buffer[idx + 1] & 0xff);
	}


	/**
	 *  Gets the doubleAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The doubleAt value
	 */
	public static double getDoubleAt(byte buffer[], int idx) {
		long l = getDoubleLongAt(buffer, idx);
		return Double.longBitsToDouble(l);
	}


	/**
	 *  Sets the doubleAt attribute of the Record class
	 *
	 *@param  buffer  The new doubleAt value
	 *@param  v       The new doubleAt value
	 *@param  idx     The new doubleAt value
	 */
	public static void setDoubleAt(byte buffer[], double v, int idx) {
		long l = Double.doubleToLongBits(v);
		setDoubleLongAt(buffer, l, idx);
	}


	/**
	 *  Gets the doubleLongAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The doubleLongAt value
	 */
	public static long getDoubleLongAt(byte buffer[], int idx) {
		long value = 0;
		//A clumsy hack, but quick
		//could not think of a better version
		//java makes is impossible to convert large bytes to chars wihtout
		//losing information

		for (int octet = 0; octet < 8; octet++) {
			for (int bit = 0; bit < 8; bit++) {
				if ((buffer[octet + idx] & (1 << bit)) != 0) {
					value |= (1L << (56 - octet * 8 + bit));

				}
			}
		}

		return value;
	}


	/**
	 *  Sets the doubleLongAt attribute of the Record class
	 *
	 *@param  buffer  The new doubleLongAt value
	 *@param  v       The new doubleLongAt value
	 *@param  idx     The new doubleLongAt value
	 */
	public static void setDoubleLongAt(byte buffer[], long v, int idx) {
		buffer[idx + 0] = (byte) ((v & 0xff00000000000000L) >> 56);
		buffer[idx + 1] = (byte) ((v & 0x00ff000000000000L) >> 48);
		buffer[idx + 2] = (byte) ((v & 0x0000ff0000000000L) >> 40);
		buffer[idx + 3] = (byte) ((v & 0x000000ff00000000L) >> 32);
		buffer[idx + 4] = (byte) ((v & 0x00000000ff000000L) >> 24);
		buffer[idx + 5] = (byte) ((v & 0x0000000000ff0000L) >> 16);
		buffer[idx + 6] = (byte) ((v & 0x000000000000ff00L) >> 8);
		buffer[idx + 7] = (byte) ((v & 0x00000000000000ffL));

	}


	/**
	 *  Sets the longAt attribute of the Record class
	 *
	 *@param  buffer  The new longAt value
	 *@param  v       The new longAt value
	 *@param  idx     The new longAt value
	 */
	public static void setLongAt(byte buffer[], long v, int idx) {
		buffer[idx + 0] = (byte) ((v & 0x00000000ff000000L) >> 24);
		buffer[idx + 1] = (byte) ((v & 0x0000000000ff0000L) >> 16);
		buffer[idx + 2] = (byte) ((v & 0x000000000000ff00L) >> 8);
		buffer[idx + 3] = (byte) ((v & 0x00000000000000ffL));
	}


	/**
	 *  Gets the longAt attribute of the Record class
	 *
	 *@param  buffer  Description of the Parameter
	 *@param  idx     Description of the Parameter
	 *@return         The longAt value
	 */
	public static long getLongAt(byte buffer[], int idx) {
		long value = 0;
		//A clumsy hack, but quick
		//could not think of a better version
		//java makes is impossible to convert large bytes to chars wihtout
		//losing information

		for (int octet = 0; octet < 4; octet++) {
			for (int bit = 0; bit < 8; bit++) {
				if ((buffer[octet + idx] & (1 << bit)) != 0) {
					value |= (1L << (24 - octet * 8 + bit));

				}
			}
		}

		return value;
	}
}

