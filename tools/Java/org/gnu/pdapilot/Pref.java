/* 
 * Pref.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 * Copyright (C) 2001 David.Goodenough@DGA.co.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

package org.gnu.pdapilot;

import java.io.*;

public class Pref extends Block {
		public int id, version;
		public boolean backup;
		public Char4 creator;
		
		public Pref() {
			super();
		}
		
		public Pref(byte[] contents, Char4 creator, int id, int version, boolean backup) {
			this.id = id;
			this.creator = creator;
			this.version = version;
			this.backup = backup;
			this.unpack(contents);
		}
		
		public String describe() {
			return " creator="+creator.toString()+", id="+id+", version="+version+
				", backup="+backup+", raw='"+Util.prettyPrint(raw)+"'";
		}
}
