/* 1.0/Util.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

public class Util {
	static public String prettyPrint(byte[] b)
		{ return prettyPrint(new String(b,0)); }
	static public String prettyPrint(Object o) {
		if (o == null) {
			return "null";
		}
		char[] c = o.toString().toCharArray();
		StringBuffer out = new StringBuffer();
		for (int i=0;i<c.length;i++) {
			if (c[i] == 13) {
				out.append("\\r");
			} else if (c[i] == 10) {
				out.append("\\n");
			} else if (c[i] == 8) {
				out.append("\\b");
			} else if (c[i] == 9) {
				out.append("\\t");
			} else if (c[i] < 32) {
				out.append('^');
				out.append((char)(c[i] ^ 64));
			} else if (c[i] > 126) {
				out.append("\\x");
				out.append(Integer.toHexString(c[i]));
			} else {
				out.append(c[i]);
			}
		}
		return out.toString();
	}
	static public String readLine() throws java.io.IOException {
		java.io.DataInputStream dis = new java.io.DataInputStream(System.in);
		return dis.readLine();
	}
}