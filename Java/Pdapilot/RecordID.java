
package Pdapilot;

public class RecordID {
	private int value;
	
	public RecordID(int id) {
		this.set(id);
	}

	public RecordID() {
		this.set(0);
	}
	
	public void set(int id) {
		value = id;
	}
	
	public int get() {
		return value;
	}
	
	public boolean equals(RecordID other) {
		return (value == other.value);
	}
	
	public String toString() {
		return "<RecordID "+value+">";
	}
}
