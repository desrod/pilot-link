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
		successfulSyncDate=ssd;
	}
    public User(String username, String pw, long uid, long vid,
        long lsp, long sec_epoch_lsd, long sec_epoch_ssd)
    {
		name=username;
		userid=uid;
		lastSyncPC=lsp;
		password=pw;
		viewerid=vid;
		lastSyncDate=new Date(sec_epoch_lsd * 1000L);
		successfulSyncDate=new Date(sec_epoch_ssd * 1000L);
    }
	public Date getLastSyncDate() {
		return lastSyncDate;
	}
	public Date getLastSuccessfulSyncDate() {
		return successfulSyncDate;
	}
    public long getLastSyncDate_time_t()
    {
        return lastSyncDate.getTime() / 1000;
    }
    public long getLastSuccessfulSyncDate_time_t()
    {
        return successfulSyncDate.getTime() / 1000;
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
