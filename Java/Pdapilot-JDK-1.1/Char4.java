/* 1.1/Char4.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

public class Char4 {
	private int value;
	private byte[] b;
	
	public Char4(int id) {
		this.set(id);
	}

	public Char4(String id) {
		this.set(id);
	}

	public Char4(byte[] b) throws java.lang.CloneNotSupportedException {
		this.set(b);
	}
	
	public Char4() {
		this.set(0);
	}
	
	public void set(int id) {
		value = id;
		b = new byte[4];
		b[3] = (byte)(id & 0xff); id >>= 8;
		b[2] = (byte)(id & 0xff); id >>= 8;
		b[1] = (byte)(id & 0xff); id >>= 8;
		b[0] = (byte)(id & 0xff); id >>= 8;
	}

	public void set(String id) {
		byte[] by = id.getBytes();
		try {
			this.set(by);
		} catch(java.lang.CloneNotSupportedException e) {
			/* Don't be silly! */
		}
	}
	
	public void set(byte[] b) throws java.lang.CloneNotSupportedException{
		this.b = (byte[])b.clone();
		value = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
	}
	
	public int getInt() {
		return value;
	}
	
	public byte[] getBytes() {
		return b;
	}

	public String getString() {
		return new String(b, 0, b.length);
	}
	
	public boolean equals(Char4 other) {
		return (value == other.value);
	}
	
	public String toString() {
		return Util.prettyPrint(b);
	}
}
