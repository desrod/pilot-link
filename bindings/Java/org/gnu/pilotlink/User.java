package org.gnu.pilotlink;
import java.util.Date;
public class User {
	String name;
	String password;
	long userid;
	long lastSyncPC;
	long viewerid;
	Date successfulSyncDate;
	Date lastSyncDate;
	public User(String username, String pw,long uid, long vid, 
	  long lsp,Date lsd, Date ssd) {
		name=username;
		userid=uid;
		lastSyncPC=lsp;
		password=pw;
		viewerid=vid;
		lastSyncDate=lsd;		
	}
	public Date getLastSyncDate() {
		return lastSyncDate;
	}
	public Date getLastSuccessfulSyncDate() {
		return successfulSyncDate;
	}
	public String getPassword() {
		return password;
	}
	public long getViewerId() {
		return viewerid;
	}
	public String getName() {
		return name;
	}
	public long getUserId() {
		return userid;
	}
	public long getLastSyncPC() {
		return lastSyncPC;
	}
}
