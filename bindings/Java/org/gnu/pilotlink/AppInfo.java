package org.gnu.pilotlink;

public abstract class AppInfo {
    protected byte buffer[];
    protected String categories[]=new String[16];
    protected boolean isCatRenamed[]=new boolean[16];
    protected int catCount=0;
    protected int dataOffset=0;
    protected int bits;
    protected long id[];
    protected int lastUniqueID;
    
    public AppInfo() {
        buffer=new byte[65535];
    }
    
    public AppInfo(AppInfo ai) {
        setBuffer(ai.getBuffer());
    }

    public abstract void setBuffer(byte[] b);
    
    public void parseCategories(){
    	bits=buffer[0]*256+buffer[1];
    	id=new long[4];
    	catCount=0;
    	for (int i=0; i<16; i++) {
    		if (buffer[2+i*16]!=0) {
    			categories[i]=getStringAt(buffer,2+i*16);
    				 //=new String(buffer,2+i*16,16);
    			
    			//System.out.println("Got Category: \""+categories[i]+"\"");
    			catCount++;
    		} else {
    			categories[i]=null;
    		}
    		if ( (bits&(1<<i))!=0) {
    			isCatRenamed[i]=true;
    		} else {
    			isCatRenamed[i]=false;
    		}
    	}    	
    	dataOffset=2+16*16;
    	for (int i=0; i<4; i++) {
    		id[i]=Record.getLongAt(buffer,dataOffset);
    		dataOffset+=4;
    	}
    	lastUniqueID=buffer[dataOffset];
    	dataOffset+=4;
    	
    	
    }
    
    public void createDefaultBuffer() {
    	for (int i=0; i<buffer.length; i++) {
    		buffer[i]=0;
    	}
    
    	bits=0;
    	for (int i=0; i<16; i++) {
    		if (isCatRenamed[i]) {
    			bits|=(1<<i);
    		}
    		if (categories[i]!=null) {
    			for (int ch=0;ch<16 && ch<categories[i].length();ch++) {
    				char z=categories[i].charAt(ch);
    				buffer[2+i*16+ch]=(byte)z;
    			}
    		}     			
    	}
    	buffer[0]=(byte) (bits>>256);
    	buffer[1]=(byte) (bits&256);
    }
    
    
    public abstract byte[] getBuffer() ;
    public boolean isCatRenamed(int idx) {
    	return isCatRenamed[idx];
    }
    public void setCatRenamed(int idx,boolean b) {
    	isCatRenamed[idx]=b;
    }
    
    public String getCatName(int idx) {
    	return categories[idx];
    }
    public void setCatName(int idx,String n){
    	if (n.length()>16) {
    		n=n.substring(0,16);
    	}
    	categories[idx]=n;
    }
    
    public static String getStringAt(byte buffer[], int idx) {
    	String str="";
    	while (idx<buffer.length && str.length()<16&&buffer[idx]!=0) {
    		
    		str+=(char)buffer[idx];
    		
    		idx++;
    	}
    	return str;
    }
    public int getCatCount() {
    	return catCount;
    }
    
    public String toString() {
    	String out="Kategories";
    	for (int i=0; i<16; i++) {
    		out+=" "+categories[i];
    		if (isCatRenamed(i)) {
    			out+="(ren)";
    		}
    	}
    	return out;
    }
}
