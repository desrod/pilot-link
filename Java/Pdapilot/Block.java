
package Pdapilot;

import java.io.*;

/** A representation of a generic database block.
 */

public class Block {
		public byte[] raw;
		
		public Block() {
			this.fill();
		}
		public Block(byte[] contents) {
			this.unpack(contents);
		}
		
		public void fill() {
		}
		
		public byte[] pack() {
			return raw;
		}
		
		public void unpack(byte[] data) {
			raw = data;
		}
		
		public String describe() {
			return "raw='" + Util.prettyPrint(raw);
		}

	public String toString() {
		return "<" + this.getClass().getName() + " " + describe() + ">";
	}
}
