package org.gnu.pilotlink;

public class AddressRecord extends Record {
	String fields[];
	int[] labelIds;
	int showPhone;
	
	public AddressRecord(Record r) {
		super(r);
	}
	
	public AddressRecord() {
		labelIds=new int[5];
		fields=new String[20];
	}
	
	public void setBuffer(byte buf[]) {
		labelIds=new int[5];
		fields=new String[20];
		if (buf.length<5) {
			System.out.println("Wrong sized buffer for address...");
			return;
		}
		showPhone=hi(buf[1]);
		
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
		setSize(buf.length);
		
	}
	public int getSize() {
		return getBuffer().length;
	}
	
	public byte[] getBuffer() {
		//throw new NullPointerException("NOT IMPLEMENTED YET, SORRY! dont call me");
		//return new byte[1];
		int size=9; //size of the flags etc.
		for (int i=0; i<fields.length; i++) {
			if (fields[i]==null) {
				continue;
			}
			size+=fields[i].length()+1; //null termination
		}
		byte buffer[]=new byte[size];
		//	System.out.println("Getbuffer (AddressRecord): created buffer of size "+size);
		buffer[1]=(byte)(((byte)showPhone <<4)|(byte)labelIds[4]);
		buffer[2]=(byte)((labelIds[3]<<4)|(byte)labelIds[2]);
		buffer[3]=(byte)((labelIds[1]<<4)|(byte)labelIds[0]);
		int p=9;
		long contents=0;
		for (int i=0; i<19; i++) {
			if (fields[i]!=null) {
				contents|=(1<<i);
				int len=fields[i].length()+1;
				Record.setStringAt(buffer,fields[i],p);
				p+=len;
			}
		}
		setLongAt(buffer,contents,4);
		return buffer;
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
		setSize(getBuffer().length);
	}
	public void setLabelId(int id, int idx) {
		if (idx>5) {
			return;
		}
		labelIds[idx]=id;
		setSize(getBuffer().length);
	}
}
