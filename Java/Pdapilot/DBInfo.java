
package Pdapilot;

public class DBInfo {
	public int flags;
	public int miscFlags;
	public Char4 creator, type;
	public int version;
	public int modnum;
	public java.util.Date createDate, modifyDate, backupDate;
	public int index;
	public String name;
	public int card;
	
	public boolean more;
	
	public String toString() {
		return "<Pdapilot.DBInfo name '"+name+"', version "+version+
			", flags "+flags+", miscFlags "+miscFlags+", creator "+creator.toString()+
			", type "+type.toString()+", modnum "+modnum+
			", createDate " + ((createDate==null) ? "null" : createDate.toString())+
			", modifiyDate " + ((modifyDate==null) ? "null" : modifyDate.toString())+
			", backupDate " + ((backupDate==null) ? "null" : backupDate.toString())+
			", card "+card+
			", more "+more+">";
	}
}

