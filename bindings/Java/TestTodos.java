import java.io.File;

import org.gnu.pilotlink.PilotLink;
import org.gnu.pilotlink.PilotLinkException;
import org.gnu.pilotlink.Record;
import org.gnu.pilotlink.SysInfo;
import org.gnu.pilotlink.ToDoRecord;
import org.gnu.pilotlink.User;

/*
 *  Created on 15.10.2004
 *
 *  TODO To change the template for this generated file go to
 *  Window - Preferences - Java - Code Style - Code Templates
 */
/**
 *@author     stephan TODO To change the template for this generated type
 *      comment go to Window - Preferences - Java - Code Style - Code Templates
 *@created    5. Dezember 2004
 */
public class TestTodos {

	/**
	 *  The main program for the TestTodos class
	 *
	 *@param  args  The command line arguments
	 */
	public static void main(String[] args) {
		String port;
		test tst = new test();
		if (args.length == 0) {
			port = "/dev/usb/tts/1";
		} else {
			port = args[0];
		}
		File p = new File(port);
		System.out.println("looking for file " + port);
		if (!p.exists()) {
			System.out.println("File does not exist... USB? Waiting for port to appear");

			while (!p.exists()) {
				System.out.print(".");
				try {
					Thread.sleep(1000);
				} catch (Exception e) {}
			}
		}
		//System.out.println("Systemzeit: "+Long.toHexString(System.currentTimeMillis()/1000));
		PilotLink pl = null;
		try {
			pl = new PilotLink(port);
			if (!pl.isConnected()) {
				System.out.println("Something went wrong. Check output!");
				System.exit(1);
			}
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
		try {
			User u = pl.getUserInfo();
			System.out.println("User: " + u.getName());
			System.out.println("Last Synchronization Date: " + u.getLastSyncDate());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			SysInfo si = pl.getSysInfo();
			System.out.println("Product ID: '" + si.getProdID() + "'");
			System.out.println("Rom Version: " + si.getRomVersion());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}

		try {
			int dbh = pl.openDB("ToDoDB");
			System.out.println("db opened!");
			System.out.println("Count: " + pl.getRecordCount(dbh));
			for (int i = 0; i < pl.getRecordCount(dbh); i++) {
				Record r = pl.getRecordByIndex(dbh, i);
				ToDoRecord tdr = new ToDoRecord(r);
				System.out.println("READ IN:");
				System.out.println(tdr);
				tst.hexdump(r.getBuffer());

				System.out.println("\nRE-PACKED:");
				tst.hexdump(tdr.getBuffer());
				System.out.println();
			}
		} catch (PilotLinkException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		try {
			pl.endSync();
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		pl.close();

	}

}

