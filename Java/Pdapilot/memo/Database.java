/* memo/Database.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.memo;

public class Database extends Pdapilot.Database {
	public Pdapilot.AppBlock newAppBlock() { return new Pdapilot.memo.AppBlock(); }
	public Pdapilot.AppBlock newAppBlock(byte[] contents) { return new Pdapilot.memo.AppBlock(contents); }
	
	public Pdapilot.Record newRecord() { return new Pdapilot.memo.Record(); }
	public Pdapilot.Record newRecord(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) 
		{ return new Pdapilot.memo.Record(contents, id, index, attr, cat); }
		
	public Pdapilot.Char4 creator() { return new Pdapilot.Char4("memo"); }
	public String dbname() { return "MemoDB"; }
	
}