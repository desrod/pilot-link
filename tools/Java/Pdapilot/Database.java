/* 
 * Database.java:
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


public class Database {
	public Record newRecord() { return new Record(); }
	public Record newRecord(RecordID id)
		{ return new Record(null, id, 0, 0, 0); }
	public Record newRecord(byte[] contents, RecordID id, int index, int attr, int cat) 
		{ return new Record(contents, id, index, attr, cat); }
		
	public Resource newResource() { return new Resource(); }
	public Resource newResource(Char4 type, int id) 
		{ return newResource(null, type, id, 0); }
	public Resource newResource(byte[] contents, Char4 type, int id, int index) 
		{ return new Resource(contents, type, id, index); }
	
	public Pref newPref() { return new Pref(); }
	public Pref newPref(byte[] contents, Char4 creator, int id, int version, boolean backup)
		{ return new Pref(contents, creator, id, version, backup); }
		
	public AppBlock newAppBlock(){ return new AppBlock(); } 
	public SortBlock newSortBlock() { return new SortBlock(); }
	public AppBlock newAppBlock(byte[] contents){ return new AppBlock(contents); } 
	public SortBlock newSortBlock(byte[] contents) { return new SortBlock(contents); }

    public Char4 creator() { return null; }
    public String dbname() { return null; }
    
    static public java.util.Hashtable dbClasses = new java.util.Hashtable(10);
    static public java.util.Hashtable prefClasses = new java.util.Hashtable(10);
    static public Database defaultDbClass;
    static public Database defaultPrefClass;
    
    static {
    	defaultDbClass = defaultPrefClass = new Database();

		Pdapilot.memo.Database memo = new Pdapilot.memo.Database();
		Pdapilot.Database.dbClasses.put(memo.dbname(), memo);
		Pdapilot.Database.prefClasses.put(memo.creator(), memo);

		Pdapilot.todo.Database todo = new Pdapilot.todo.Database();
		Pdapilot.Database.dbClasses.put(todo.dbname(), todo);
		Pdapilot.Database.prefClasses.put(todo.creator(), todo);

		Pdapilot.mail.Database mail = new Pdapilot.mail.Database();
		Pdapilot.Database.dbClasses.put(mail.dbname(), mail);
		Pdapilot.Database.prefClasses.put(mail.creator(), mail);

		Pdapilot.appointment.Database appointment = new Pdapilot.appointment.Database();
		Pdapilot.Database.dbClasses.put(appointment.dbname(), appointment);
		Pdapilot.Database.prefClasses.put(appointment.creator(), appointment);

		Pdapilot.expense.Database expense = new Pdapilot.expense.Database();
		Pdapilot.Database.dbClasses.put(expense.dbname(), expense);
		Pdapilot.Database.prefClasses.put(expense.creator(), expense);

		Pdapilot.address.Database address = new Pdapilot.address.Database();
		Pdapilot.Database.dbClasses.put(address.dbname(), address);
		Pdapilot.Database.prefClasses.put(address.creator(), address);

    }

}
