
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
