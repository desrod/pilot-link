/* mail/Database.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot.mail;

public class Database extends Pdapilot.Database {
	public Pdapilot.AppBlock newAppBlock() { return new Pdapilot.mail.AppBlock(); }
	public Pdapilot.AppBlock newAppBlock(byte[] contents) { return new Pdapilot.mail.AppBlock(contents); }
	
	public Pdapilot.Record newRecord() { return new Pdapilot.mail.Record(); }
	public Pdapilot.Record newRecord(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) 
		{ return new Pdapilot.mail.Record(contents, id, index, attr, cat); }

	public Pdapilot.Pref newPref() { return new Pdapilot.Pref(); }
	public Pdapilot.Pref newPref(byte[] contents, Pdapilot.Char4 creator, int id, int version, boolean backup)
	{ 
		if ((id == 1) || (id == 2)) 
			return new Pdapilot.mail.SyncPref(contents, creator, id, version, backup);
		else if (id == 3)
			return new Pdapilot.mail.SignaturePref(contents, creator, id, version, backup);
		else
			return new Pdapilot.Pref(contents, creator, id, version, backup);
	}
	
	public Pdapilot.Pref newSyncPref() { return new Pdapilot.mail.SyncPref(); }
	public Pdapilot.Pref newSyncPref(boolean local ) { return new Pdapilot.mail.SyncPref(local); }
	public Pdapilot.Pref newSignaturePref() { return new Pdapilot.mail.SignaturePref(); }
		
	public Pdapilot.Char4 creator() { return new Pdapilot.Char4("mail"); }
	public String dbname() { return "MailDB"; }
	
}
