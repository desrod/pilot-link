package org.gnu.pilotlink;
import java.util.*;
public abstract class Record {
    public final static int DELETED=0x80;
    public final static int DIRTY=0x40;
    public final static int BUSY=0x20;
    public final static int SECRET=0x10;
    public final static int ARCHIVED=0x8;
    
    protected long id;
    protected int attribs;
    protected int category;
    protected int size;
    
    public Record() {
        id=0;
        attribs=0;
        category=0;
        size=0;
    }
    
    public boolean isDeleted() {
        return (attribs&DELETED)!=0;
    }
    public boolean isSecret() {
        return (attribs&SECRET)!=0;
    }
    public boolean isArchived() {
        return (attribs&ARCHIVED)!=0;
    }
    public boolean isDirty() {
        return (attribs&DIRTY)!=0;
    }
    public boolean isBusy() {
        return (attribs&BUSY)!=0;
    }
    public void setDirty(boolean d) {
        if (d) {
            attribs=attribs|DIRTY;
        } else {
            attribs=attribs&(~DIRTY);
        }
    }
    public void setArchived(boolean a) {
        if (a) {
            attribs=attribs|ARCHIVED;
        } else {
            attribs=attribs&(~ARCHIVED);
        }
    }
    public Record(int i, int att, int cat, int sz) {
        size=sz;
        id=i;
        attribs=att;
        category=cat;
    }
    public Record(Record r) {
        size=r.getSize();
        id=r.getId();
        attribs=r.getAttribs();
        category=r.getCategory();
        setBuffer(r.getBuffer());
    }
    
    public void setId(long i) {
		id=i;
	}
	public int getCategory() {
		return category;
	}
    public void setCategory(int c) {
        category=c;
    }
    public void setAttribs(int a) {
        attribs=a;
    }
	public int getAttribs() {
		return attribs;
	}
	public long getId() {
		return id;
	}
	public int getSize() {
		return size;
	}
    public void setSize(int s) {
        size=s;
    }
	public abstract byte[] getBuffer();
    public abstract void setBuffer(byte buf[]);
    
    public Date getDateAt(int idx) {
        return getDateAt(getBuffer(),idx);
    }
    
	public static Date getDateAt(byte buffer[], int idx) {        
		int binary=buffer[idx]*256+buffer[idx+1];
		int year=binary & 0xfe00;
		year=year>>9;
		year+=1904;
		int month=binary & 0x01e0;
		month = month >>5;
		month--;
		int day=binary & 0x001f;
		//System.out.println("Y: "+year+" M:"+month+" D: "+day);
		GregorianCalendar cal=new GregorianCalendar(year,month,day);
		return cal.getTime();
	}
    public static void setDateAt(byte buffer[], Date d, int idx) {
        GregorianCalendar cal=new GregorianCalendar();
        //System.out.println("Setting date "+d+" at: "+idx);
        cal.setTime(d);
        int binary=0; int year=cal.get(cal.YEAR); int month=cal.get(cal.MONTH);
        int day=cal.get(cal.DAY_OF_MONTH);
        year-=1904; year=year<<9;
        month++;
        month=month<<5;
        
        binary=year|month|day;
        
        buffer[idx+1]=(byte) (binary & 0x00ff);
        buffer[idx]=(byte) ((binary&0xff00)>>8);
    }
    public static void setDateTimeAt(byte buffer[], Date d, int idx, int tidx) {
        setDateAt(buffer,d,idx);
        GregorianCalendar cal=new GregorianCalendar();
        cal.setTime(d);
        int h=cal.get(cal.HOUR_OF_DAY);
        int m=cal.get(cal.MINUTE);
        buffer[tidx]=(byte)h;
        buffer[tidx+1]=(byte)m;
    }
    
    public Date getDateTimeAt(int idx, int tidx) {
        return getDateTimeAt(getBuffer(), idx,tidx);
    }
	public static Date getDateTimeAt(byte buffer[],int idx, int tidx) {
		int binary=buffer[idx]*256+buffer[idx+1];
		int year=binary & 0xfe00;
		year=year>>9;
		year+=1904;
		int month=binary & 0x01e0;
		month = month >>5;
		month--;
		int day=binary & 0x001f;
		//System.out.println("Y: "+year+" M:"+month+" D: "+day);
		int hour=buffer[tidx];
		int min=buffer[tidx+1];
		GregorianCalendar cal=new GregorianCalendar(year,month,day, hour, min, 0);
		return cal.getTime();
	}
    public static int setStringAt(byte buffer[], String s, int idx) {
        byte str[]=s.getBytes();
        for (int i=0; i<str.length;i++) {
            buffer[idx+i]=str[i];
        }
        buffer[idx+str.length]=0;
        return (idx+str.length+1);
    }
    public String getStringAt(int idx) {
        return getStringAt(getBuffer(),idx);
    }
    public static String getStringAt(byte buffer[], int idx) {
        String str="";
        while (idx<buffer.length && buffer[idx]!=0) {
            str+=(char)buffer[idx++];
        }
        return str;
    }
    
    public static void setIntAt(byte buffer[], int i, int idx) {
        buffer[idx+1]=(byte) (i&0x00ff);
        buffer[idx]=(byte) ((i&0xff00)>>8);
    }
    public int getIntAt(int idx) {
        return getIntAt(getBuffer(),idx);
    }
    public static int getIntAt(byte buffer[], int idx) {
     
        return buffer[idx]*256+buffer[idx+1];
    }
}
