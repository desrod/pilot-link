/* UserInfo.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

public class UserInfo {
	public int userID, viewerID, lastSyncPC;
	public java.util.Date successfulSyncDate, lastSyncDate;
	public String username;
	public byte[] password;
	
	public String toString() {
		return "<Pdapilot.UserInfo userID "+userID+", viewerID "+viewerID+", lastSyncPC "+lastSyncPC+
			", successfulSyncDate "+((successfulSyncDate == null) ? "null" : successfulSyncDate.toString())+", syncLast "+((lastSyncDate == null) ? "null" : lastSyncDate.toString())+
			", username '"+username+"', Util.prettyPrint(password) '"+"'>";
	}
}

