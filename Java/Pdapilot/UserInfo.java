
package Pdapilot;

public class UserInfo {
	public int userID, viewerID, lastSyncPC;
	public java.util.Date syncSuccess, syncLast;
	public String username;
	public byte[] password;
	
	public String toString() {
		return "<Pdapilot.UserInfo userID "+userID+", viewerID "+viewerID+", lastSyncPC "+lastSyncPC+
			", syncSuccess "+((syncSuccess == null) ? "null" : syncSuccess.toString())+", syncLast "+((syncLast == null) ? "null" : syncLast.toString())+
			", username '"+username+"', Util.prettyPrint(password) '"+"'>";
	}
}

