/* 
 * expense/Database.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
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

package Pdapilot.expense;

public class Database extends Pdapilot.Database {
	public Pdapilot.AppBlock newAppBlock() { return new Pdapilot.expense.AppBlock(); }
	public Pdapilot.AppBlock newAppBlock(byte[] contents) { return new Pdapilot.expense.AppBlock(contents); }
	
	public Pdapilot.Record newRecord() { return new Pdapilot.expense.Record(); }
	public Pdapilot.Record newRecord(byte[] contents, Pdapilot.RecordID id, int index, int attr, int cat) 
		{ return new Pdapilot.expense.Record(contents, id, index, attr, cat); }

	public Pdapilot.Pref newPref() { return new Pdapilot.expense.Pref(); }
	public Pdapilot.Pref newPref(byte[] contents, Pdapilot.Char4 creator, int id, int version, boolean backup)
	{ 
		if (id == 1) 
			return new Pdapilot.expense.Pref(contents, creator, id, version, backup);
		else
			return new Pdapilot.Pref(contents, creator, id, version, backup);
	}
	
	public Pdapilot.Char4 creator() { return new Pdapilot.Char4("exps"); }
	public String dbname() { return "ExpenseDB"; }
	
}
