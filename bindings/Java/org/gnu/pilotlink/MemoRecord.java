package org.gnu.pilotlink;

public class MemoRecord extends Record {
    String memo;
    
    /**
    * Calls the constructor of Record which calles setBuffer,
    * do not call setBuffer directly
    */
    public MemoRecord(Record r) {
        super(r);
    }
    public MemoRecord(String txt) {
        memo=txt+'\0';
        setSize(memo.length());
    }
    public String getText() {
        return memo;
    }
    public void setText(String m) {
        memo=m;
        setSize(m.length()+1);
    }
    
    
    /**
    * usually called by the Constructor of Record
    */
    public void setBuffer(byte dat[]) {
        memo=new String(dat)+'\0';
        setSize(memo.length()+1);
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
