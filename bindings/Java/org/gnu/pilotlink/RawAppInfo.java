package org.gnu.pilotlink;


public class RawAppInfo extends AppInfo {
    byte buffer[];
    String categories[]=new String[16];
    boolean isRenamed[]=new boolean[16];
    int catCount=0;
    
    public RawAppInfo(byte b[]) {
        setBuffer(b);
    }
    
    public byte[] getBuffer() {
        return buffer;
    }
    
    public void setBuffer(byte b[]) {
        buffer=b;
        int bits=b[0]*256+b[1];
       
        catCount=0;
        for (int i=0; i<16; i++) {
            if (b[2+i*16]!=0) {
                categories[i]=new String(b,2+i*16,16);
                System.out.println("Got Category: \""+categories[i]+"\"");
                catCount++;
            } else {
                categories[i]=null;
            }
            if ( (bits&(1<<i))!=0) {
                isRenamed[i]=true;
            } else {
                isRenamed[i]=false;
            }
        }
        
    }

    public boolean isCatRenamed(int idx) {
        return isRenamed[idx];
    }
    
    public String getCatName(int idx) {
        return categories[idx];
    }
    public int getCatCount() {
        return catCount;
    }
}
