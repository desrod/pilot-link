
package Pdapilot.expense;

public class distance {
	private int idx;
	
	final static private String[] names = 
		{ "Miles", "Kilometers" };

	public static distance Miles = new distance(0);
	public static distance Kilometers =  new distance(1);
	
	private static distance[] objs;
	
	private distance(int value) {
		this.idx = value;
	}
	
	public static distance get(int value) {
		return objs[value];
	}
	public static distance get(String value) {
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
		return "distance."+names[idx];
	}
		
	public int getValue() {
		return idx;
	}
	public String getName() {
		return names[idx];
	}

	static {
		objs = new distance[2];
		objs[0] = distance.Miles;
		objs[1] = distance.Kilometers;
	}
};