/* 
 * RecordID.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

package Pdapilot;

public class RecordID {
	private int value;
	
	public RecordID(int id) {
		this.set(id);
	}

	public RecordID() {
		this.set(0);
	}
	
	public void set(int id) {
		value = id;
	}
	
	public int get() {
		return value;
	}
	
	public boolean equals(RecordID other) {
		return (value == other.value);
	}
	
	public String toString() {
		return "<RecordID "+value+">";
	}
}
