
package Pdapilot;

public class end {
	private int idx;
	
	static private String[] names = null;
	static private end[] objs = null;

	public static end Normal = new end(0, "Normal");
	public static end OutOfMemory =  new end(1, "OutOfMemory");
	public static end UserCancelled =  new end(2, "UserCancelled");
	public static end Other =  new end(3, "Other");
	
	private end(int value, String name) {
		if (objs == null)
			objs = new end[4];
		if (names == null)
			names = new String[4];
		this.idx = value;
		objs[value] = this;
		names[value] = name;
	}
	
	public static end get(int value) {
		return objs[value];
	}
	public static end get(String value) {
		int i;
		for(i=0;i<names.length;i++)
			if (names[i].equals(value))
				return objs[i];
		return null;
	}
	public static String[] getNames() {
		return names;
	}
	
	public String toString() {
		return "end."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}
};