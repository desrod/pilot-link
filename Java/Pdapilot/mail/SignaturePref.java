/* mail/SignaturePref.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.mail;

import java.io.*;

public class SignaturePref extends Pdapilot.Pref {
	public String signature;

	public native void unpack(byte[] data);
	public native byte[] pack();
		
		public SignaturePref() {
			super(null, new Pdapilot.Char4("mail"), 3, 1, true);
		}
		
		public SignaturePref(byte[] contents, Pdapilot.Char4 creator, int id, int version, boolean backup) {
			super(contents, creator, id, version, backup);
		}
		
		public String describe() {
			return "signature='"+signature+"', "+super.describe();
		}
}
