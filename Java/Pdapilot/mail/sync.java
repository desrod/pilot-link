
package Pdapilot.mail;

public class sync {
	private int idx;
	
	final static private String[] names = 
		{ "All", "Send", "Filter" };

	public static sync All = new sync(0);
	public static sync Send =  new sync(1);
	public static sync Filter = new sync(2);
	
	private static sync[] objs;
	
	private sync(int value) {
		this.idx = value;
	}
	
	public static sync get(int value) {
		return objs[value];
	}
	public static sync get(String value) {
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
		return "sync."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new sync[3];
		objs[0] = sync.All;
		objs[1] = sync.Send;
		objs[2] = sync.Filter;
	}
};