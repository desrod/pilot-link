package org.gnu.pilotlink;

public class AddressRecord extends Record {
	String fields[];
	int[] labelIds;
    
	public AddressRecord(Record r) {
		super(r);
	}

	public AddressRecord() {
	}

    public void setBuffer(byte buf[]) {
        labelIds=new int[5];
        fields=new String[20];
        int showPhone=hi(buf[1]);
        labelIds[0]=0;
        
        labelIds[1]=0;
        labelIds[4]=lo(buf[1]);
        labelIds[3]=hi(buf[2]);
        labelIds[2]=lo(buf[2]);
        labelIds[1]=hi(buf[3]);
        labelIds[0]=lo(buf[3]);
        
        long contents=buf[4]*256*256*256+buf[5]*256*256+buf[6]*256+buf[7];
        int len=buf.length-9;
        int offset=9;
        
        for (int i=0; i<19; i++) {
            if ((contents & (1<<i))!=0) { 
                if (len<1) { 
                    return;
                } 
                fields[i]=getStringAt(buf,offset);
                
                len-=fields[i].length()+1;
                offset+=fields[i].length()+1;
            } else {
                
                fields[i]=null;
            }
            
        }
    }

	public byte[] getBuffer() {
        throw new NullPointerException("NOT IMPLEMENTED YET, SORRY! dont call me");
		//return new byte[1];
	}
    
    private int hi(byte b) {
        return (b&0xf0)>>4;
    }
    private int lo(byte b) {
        return (b&0x0f);
    }
    
    public String getField(int idx) {
        return fields[idx];
    }
    public int getLabelId(int idx) {
        if (idx>5) {
            return 0;
        }
        return labelIds[idx];
    }
    
    public void setField(String cont,int idx) {
        if (idx>19) {
            return;
        }
        fields[idx]=cont;
        
    }
    public void setLabelId(int id, int idx) {
        if (idx>5) {
            return;
        }
        labelIds[idx]=id;
    }
}
