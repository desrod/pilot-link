
package Pdapilot;


public class Database {
	public Record newRecord() { return new Record(); }
	public Record newRecord(byte[] contents, RecordID id, int index, int attr, int cat) 
		{ return new Record(contents, id, index, attr, cat); }
		
	public Resource newResource() { return new Resource(); }
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
    }

}
