/* appointment/Database.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.appointment;

public class Database extends Pdapilot.Database {
	public Pdapilot.AppBlock newAppBlock() { return new Pdapilot.appointment.AppBlock(); }
	public Pdapilot.AppBlock newAppBlock(byte[] contents) { return new Pdapilot.appointment.AppBlock(contents); }
	
	public Pdapilot.Record newRecord() { return new Pdapilot.appointment.Record(); }
	public Pdapilot.Record newRecord(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) 
		{ return new Pdapilot.appointment.Record(contents, id, index, attr, cat); }
		
	public Pdapilot.Char4 creator() { return new Pdapilot.Char4("date"); }
	public String dbname() { return "DatebookDB"; }
	
}