package org.gnu.pilotlink;

public abstract class AppInfo {
    protected byte buffer[];
    
    
    public AppInfo() {
        buffer=new byte[65535];
    }
    
    public AppInfo(AppInfo ai) {
        setBuffer(ai.getBuffer());
    }

    public abstract void setBuffer(byte[] b);
    
    public byte[] getBuffer() {
        return buffer;
    }
}
