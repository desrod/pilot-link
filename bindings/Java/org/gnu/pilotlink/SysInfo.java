package org.gnu.pilotlink;

public class SysInfo {
   private long romVersion;
   private long locale;
   private String prodID;
   private short dlpMajorVersion;
   private short dlpMinorVersion;
   private short compatMajorVersion;
   private short compatMinorVersion;
   private long maxRecSize;

   public SysInfo(String pID, long rv, long lc, short mav, short miv, short cmav, short cmiv, long maxr) {
	romVersion=rv;
	locale=lc;
	prodID=pID;
	dlpMajorVersion=mav;
	dlpMinorVersion=miv;
	compatMajorVersion=cmav;
	compatMinorVersion=cmiv;
	maxRecSize=maxr;
   }
   public String getProdID() {
   	return prodID;
   }
   public long getMaxRecSize() {
   	return maxRecSize;
   }
   public long getCompatMajorVersion() {
   	return compatMajorVersion;
   }
   public long getCompatMinorVersion() {
   	return compatMinorVersion;
   }
   public long getMinorVersion() {
   	return dlpMinorVersion;
   }
   public long getMajorVersion() {
      	return dlpMajorVersion;
   }
   public long getLocale() {
   	return locale;
   }
   public long getRomVersion() {
   	return romVersion;
   }
}
