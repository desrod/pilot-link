package org.gnu.pilotlink;

public class MemoRecord extends Record {
    String memo;
    
    public MemoRecord(Record r) {
        super(r);
    }
    public MemoRecord(String txt) {
        memo=txt;
        setSize(memo.length());
    }
    public String getText() {
        return memo;
    }
    public void setText(String m) {
        memo=m;
        setSize(m.length()+1);
    }
    
    public void setBuffer(byte dat[]) {
        memo=new String(dat);
        setSize(memo.length());
    }
    public byte[] getBuffer() {
        //Trying to write a Memo!
		//String str="Eine TestNachricht!";
		//byte a[]=str.getBytes();
		//Record r=new Record(a,a.length,64,2);
		//pl.writeRecord(dbh,r);
        return memo.getBytes();
    }
}
