
package Pdapilot;

public class DBInfo {
	public int flags;
	public int miscflags;
	public Char4 creator, type;
	public int version;
	public int modnum;
	public java.util.Date creation, modification, backup;
	public int index;
	public String name;
	public int card;
	
	public boolean more;
	
	public String toString() {
		return "<Pdapilot.DBInfo name '"+name+"', version "+version+
			", flags "+flags+", miscflags "+miscflags+", creator "+creator.toString()+
			", type "+type.toString()+", modnum "+modnum+
			", creation " + ((creation==null) ? "null" : creation.toString())+
			", modification " + ((modification==null) ? "null" : modification.toString())+
			", backup " + ((backup==null) ? "null" : backup.toString())+
			", card "+card+
			", more "+more+">";
	}
}

