package org.gnu.pilotlink;
import java.util.Date;
import java.util.GregorianCalendar;
public class DatebookRecord extends Record {
    byte buffer[]=new byte[65535]; //max buffer
    //Statics
    public final static int REP_WEEKLY=2;
    public final static int REP_DAYLY=1;
    public final static int REP_MONTHLY_BY_DAY=3;
    public final static int REP_MONTHLY_BY_DATE=4;
    public final static int REP_YEARLY=5;
    public final static int REP_NONE=6;
    public final static int NO_REP=0;
    
    public final static int ALARM_MINUTES=0;
    public final static int ALARM_HOURS=1;
    public final static int ALARM_DAYS=2;
    public final static int ALARM_NONE=3;
    
    //Privates
    //private Record rawData;
    private Date startDate;
    private Date endDate;
    private Date repeatEnd;
    
    private String description;
    private String note;
    
    
    private int alarmAdvance;
    private int alarmUnits;
    
    private int repAdvance;
    private int repType;
    private int repDay;
    private int repWeekstart;
    private int dist;
    
    private Date repExceptions[];
    
    private boolean[] repDays;
    
    //die flags...
    private boolean hasNote;
    private boolean hasAlarm;
    private boolean hasTime;
    private boolean isRepeated;
    private boolean hasDescription;
    private boolean hasExceptions;
    private boolean repeatForever;
    
    public boolean hasNote() {
        return hasNote;
    }
    public boolean hasTime() {
        return hasTime;
    }
    public boolean hasAlarm() {
        return hasAlarm;
    }
    public boolean isRepeated() {
        return isRepeated;
    }
    public boolean hasDescription() {
        return hasDescription;
    }
    public boolean hasExceptions() {
        return hasExceptions;
    }
    public boolean repeatForever() {
        return repeatForever;
    }
    
    public DatebookRecord(Date sd, Date ed,String descr, String n) {
        resetVars();

        
        startDate = sd;
        endDate=ed;
        description=descr;
        note=n;
        
        if (n!=null && n.length()!=0) {
            hasNote=true;
        }
        if (descr!=null && descr.length()!=0) {
            hasDescription=true;
        }
        GregorianCalendar cal=new GregorianCalendar();
        cal.setTime(sd);
        if (cal.get(GregorianCalendar.HOUR_OF_DAY)!=0 && cal.get(GregorianCalendar.MINUTE)!=0) {
            hasTime=true;
        }
        
        //CLUMSY: getting buffer once to calc size
        //getBuffer();
	setSize(getBuffer().length);
    }
    //Constructor
    public DatebookRecord(Record raw) {
        super(raw);        
    }
    public DatebookRecord() {
	    startDate=new Date();
	    resetVars();
	    endDate=new Date();
    }
    
    private void resetVars() {
        description="";
        note="";
        
        
        alarmAdvance=0;
        alarmUnits=ALARM_NONE;
        
        repAdvance=0;
        repType=REP_NONE;
        repDay=0;
        repWeekstart=0;

        repAdvance=0;
        dist=0;
        repDays= new boolean[] { false, false ,false, false, false, false, false };
        hasNote=false;
        hasAlarm=false;
        hasTime=false;
        isRepeated=false;
        hasDescription=false;
        hasExceptions=false;
        repeatForever=false;
    }
    public void setBuffer(byte arr[]) {
        //buffer=raw.getBuffer();
        resetVars();
        if (arr[0]==(byte)0xff && arr[1]==(byte)0xff) {
            //keine Zeit festgelegt
            hasTime=false;
            startDate=Record.getDateAt(arr,4);
            endDate=startDate;			
        } else {
            hasTime=true;
            startDate=Record.getDateTimeAt(arr,4,0);
            endDate=Record.getDateTimeAt(arr,4,2);
        }
        
        //Parsing flags
        int flags=arr[6];
        if ((flags & 64) != 0) {
            hasAlarm=true;
        } 
        if ((flags & 32) != 0) {
            isRepeated=true;
        }
        if ((flags & 8) !=0) {
            hasExceptions=true;
        }
        if ((flags & 16) !=0) {
            hasNote=true;
        }
        if ((flags & 4) != 0) {
            hasDescription=true;
        }
        
        int idx=8;
        //getting data
        if (hasAlarm) {
            alarmAdvance=arr[idx];
            idx++;
            alarmUnits=arr[idx];
            idx++;
        }
        if (isRepeated) {
            repType=arr[idx];
            idx+=2;
            dist=256*arr[idx]+arr[idx]+1;
            
            if (dist==0xffff) {
                repeatForever=true;
            } else {
                repeatEnd=Record.getDateAt(arr,idx);
            }
            idx+=2;
            repAdvance=arr[idx];
            idx++;
            int on=arr[idx];
            if (repType==REP_MONTHLY_BY_DAY) {
                repDay=on;
            } else {
                for (int i=0; i<7;i++) {
                    //repDays[i]=((on&(1<<i))!=0);
                }
            }
            idx++;
            repWeekstart=arr[idx];
            idx+=2;
            
        }
        
        if (hasExceptions) {
            repExceptions=new Date[arr[idx]];
            idx+=2;
            for (int i=0; i<repExceptions.length; i++, idx+=2) {
                repExceptions[i]=Record.getDateAt(arr,idx);
            }
        }
        //idx=0x10; //always at idx 16? cant set exceptions on my palm
        if (hasDescription) {
            description=Record.getStringAt(arr,idx);
            idx+=description.length()+1;
        }
        if (hasNote) {
            note=Record.getStringAt(arr,idx);
            idx+=note.length()+1;
        }
        setSize(idx);
    }
    public byte[] getBuffer() {
        int bytecount=0;
        
        
        if (!hasTime) {
            buffer[0]=(byte)0xff;
            buffer[1]=(byte)0xff;
            buffer[2]=(byte)0xff;
            buffer[3]=(byte)0xff;
            Record.setDateAt(buffer,startDate,4);
        } else {
            Record.setDateTimeAt(buffer,startDate,4,0);
            Record.setDateTimeAt(buffer,endDate,4,2);
        }
        int flags=0;
        if (hasAlarm) flags=flags|64;
        if (isRepeated) flags=flags|32;
        if (hasExceptions) flags=flags|8;
        if (hasNote) flags=flags|16;
        if (hasDescription) flags=flags|4;
        buffer[6]=(byte)flags;
        bytecount=8;
        if (hasAlarm) {
            buffer[bytecount]=(byte)alarmAdvance;
            bytecount++;
            buffer[bytecount]=(byte)alarmUnits;
            bytecount++;
        }
        if (isRepeated) {
            buffer[bytecount]=(byte)repType;
            bytecount+=2;
            if (repeatForever) {
                buffer[bytecount]=(byte)0xff;
                buffer[bytecount+1]=(byte)0xff;
            } else {
                Record.setIntAt(buffer,dist-1, bytecount);
            }
            bytecount+=2;
            buffer[bytecount]=(byte)repAdvance;
            bytecount++;
            if (repType==REP_MONTHLY_BY_DAY) {
                buffer[bytecount]=(byte)repDay;
            } else {
                int days=0;
                for (int i=0; i<7; i++) {
                    if (repDays[i]) {
                        days=days|(1<<i);
                    }
                }
            }
            bytecount++;
            buffer[bytecount]=(byte)repWeekstart;
            bytecount+=2;
        }
        if (hasExceptions) {
            buffer[bytecount]=(byte)repExceptions.length;
            bytecount+=2;
            for (int i=0;i<repExceptions.length;i++, bytecount+=2) {
                Record.setDateAt(buffer,repExceptions[i],bytecount);
            }
        }
        //bytecount=16; //cant set exceptions on my palm...
        if (hasDescription) {
            bytecount=Record.setStringAt(buffer,description,bytecount);
        }
        if (hasNote) {
            bytecount=Record.setStringAt(buffer,note,bytecount);
        }
        byte ret[]=new byte[bytecount];
        for (int i=0; i<bytecount; i++) {
            ret[i]=buffer[i];
        }
        
        return ret;
    }
    
    public Date getStartDate() {
        return startDate;
    }
    public Date getEndDate() {
        return endDate;
    }
    public Date repeatUntil() {
        return repeatEnd;
    }
    public String getDescription() {
        return description;
    }
    
    public String getNote() {
        return note;
    }
    public Date getRepeatEnd() {
        return repeatEnd;
    }
    public Date[] getRepExceptions() {
        return repExceptions;
    }
    public int getRepWeekstart() {
        return repWeekstart;
    }
    public int getRepDay() {
        return repDay;
    }
    public int getRepType() {
        return repType;
    }
    public int getRepAdvance() {
        return repAdvance;
    }
    public int getAlarmAdvance() {
        return alarmAdvance;
    }
    public int getAlarmUnits() {
        return alarmUnits;
    }
    public boolean[] getRepDays() {
        return repDays;
    }
    
    public void setStartDate(Date startDate) {
        this.startDate = startDate;
	
	setSize(getBuffer().length);
    }
    public void setEndDate(Date endDate) {
        this.endDate = endDate;
	if (endDate!=null)
		hasTime=true;
    
	setSize(getBuffer().length);
    }
    
    public void setRepeated(boolean r) {
	    isRepeated=r;
    }
    
    public void setRepeatForever(boolean r) {
	    repeatForever=r;
    }
    
    public void setRepeatEnd(Date repeatEnd) {
        this.repeatEnd = repeatEnd;
	if (repeatEnd!=null) {
		setRepeatForever(false);
	}
	setSize(getBuffer().length);
    }
    public void setDescription(String description) {
        this.description = description;
	if (description!=null) {
		hasDescription=true;
	} else {
		hasDescription=false;
	}
	setSize(getBuffer().length);
    }
    public void setNote(String note) {
        this.note = note;
	if (note!=null) {
		hasNote=true;
	} else {
		hasNote=false;
	}
	setSize(getBuffer().length);
    }
    
    public void setAlarm(boolean a) {
	    hasAlarm=a;
    }
    
    public void setAlarmAdvance(int alarmAdvance) {
        this.alarmAdvance = alarmAdvance;
	setSize(getBuffer().length);
    }
    public void setAlarmUnits(int alarmUnits) {
        this.alarmUnits = alarmUnits;
	setSize(getBuffer().length);
    }
    public void setRepAdvance(int repAdvance) {
        this.repAdvance = repAdvance;
	setSize(getBuffer().length);
    }
    public void setRepType(int repType) {
        this.repType = repType;
	setSize(getBuffer().length);
    }
    public void setRepDay(int repDay) {
        this.repDay = repDay;
	setSize(getBuffer().length);
    }
    public void setRepWeekstart(int repWeekstart) {
        this.repWeekstart = repWeekstart;
	setSize(getBuffer().length);
    }
    public void setRepExceptions(Date repExceptions[]) {
        this.repExceptions = repExceptions;
	setSize(getBuffer().length);
    }
    public void setRepDays(boolean[] repDays) {
        this.repDays = repDays;
	setSize(getBuffer().length);
    }
    
}
