import org.gnu.pilotlink.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;
/**
 *  Description of the Class
 *
 *@author     stephan
 *@created    5. Dezember 2004
 */
public class test {
	/**
	 *  Description of the Method
	 *
	 *@param  args  Description of the Parameter
	 */
	public static void main(String args[]) {
		JFrame frame = new JFrame("Progress");
		frame.getContentPane().setLayout(new BorderLayout());
		JLabel txt = new JLabel("Reading in");
		frame.getContentPane().add(txt, BorderLayout.NORTH);
		JProgressBar progress = new JProgressBar(0, 100);
		frame.getContentPane().add(progress, BorderLayout.CENTER);
		frame.setSize(300, 50);
		frame.setVisible(true);

		Date yesterday = new Date(System.currentTimeMillis() - 3600000 * 24);
		System.out.println("Date 2 check: " + yesterday);
		String port;
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

		System.out.println("GEtting App...");
		pl.getAppInfoBlock("AddressDB");
		if (true) {
			try {
				pl.endSync();
				pl.close();
				System.exit(1);
			} catch (Exception e) {}
		}
		try {
			SysInfo si = pl.getSysInfo();
			System.out.println("Product ID: '" + si.getProdID() + "'");
			System.out.println("Rom Version: " + si.getRomVersion());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			System.out.println("Opening catdb!");
			int dbh = pl.openDB("DatebookDB");
			System.out.println("db opened!");
			System.out.println("Count: " + pl.getRecordCount(dbh));
			progress.setMaximum(pl.getRecordCount(dbh));
			Vector ids = new Vector();

			for (int i = 0; i < pl.getRecordCount(dbh); i++) {
				Record r = pl.getRecordByIndex(dbh, i);
				progress.setValue(i);
				txt.setText("Reading in " + i);
				if (r == null) {
					break;
				}
				if (r.getBuffer() == null || r.getBuffer().length == 0) {
					System.out.println("0-sized record? Deleting...");
					pl.deleteRecordById(dbh, r.getId());
					continue;
				}
				if (!r.isDirty()) {
					//only new entries
					continue;
				}
				DatebookRecord dbr = new DatebookRecord(r);
				//System.out.println("comparing "+dbr.getStartDate()+ "to yesterday "+yesterday);
				//if (dbr.getStartDate().before(yesterday) || dbr.getStartDate().after(new Date())) {
				//    continue;
				//}
				ids.add(new Integer(i));
				System.out.println("Read index " + i);
				System.out.println("  id: " + dbr.getId());
				System.out.println("attr: " + Integer.toHexString(dbr.getAttribs()));
				System.out.println(" dirty:" + dbr.isDirty());
				System.out.println(" arch :" + dbr.isArchived());
				System.out.println(" del  :" + dbr.isDeleted());
				System.out.println("size: " + dbr.getSize());
				System.out.println("cat : " + dbr.getCategory());
				hexdump(dbr.getBuffer());
				hexdump(r.getBuffer());
				System.out.println("Has alarm  : " + dbr.hasAlarm());
				System.out.println("Description: " + dbr.hasDescription());
				System.out.println("Note       : " + dbr.hasNote());
				System.out.println("Reapeated  : " + dbr.isRepeated());
				System.out.println("hasTime    : " + dbr.hasTime());
				if (dbr.hasDescription()) {
					System.out.println("Descr: " + dbr.getDescription());
				}
				if (dbr.hasNote()) {
					System.out.println("Note: " + dbr.getNote());
				}
				System.out.println("Start: " + dbr.getStartDate());
				System.out.println("End: " + dbr.getEndDate());

			}
			//Doing this not to interfere with the reading of records
			for (int i = 0; i < ids.size(); i++) {
				System.out.println("updating..." + ids.get(i));
				Record r = pl.getRecordByIndex(dbh, ((Integer) ids.get(i)).intValue());
				r.setDirty(false);
				hexdump(r.getBuffer());
				if (r.isDirty()) {
					System.out.println("H\ufffd\ufffd\ufffd\ufffd?!");
				}
				//pl.writeRecord(dbh,r);
			}

			System.out.println("\n\n\nNew Datebookentry:\n");
			GregorianCalendar start = new GregorianCalendar();
			start.setTime(new Date());
			start.set(GregorianCalendar.SECOND, 0);
			GregorianCalendar end = new GregorianCalendar();
			end.setTime(new Date(System.currentTimeMillis() + 15 * 60000));
			end.set(GregorianCalendar.SECOND, 0);
			DatebookRecord dbr = new DatebookRecord(start.getTime(), end.getTime(), "Test of pilot-links new java bindings", "");
			//hexdump(dbr.getBuffer());
			//System.out.println("Size: "+dbr.getSize());

			//pl.writeRecord(dbh,dbr);
			pl.closeDB(dbh);

		} catch (PilotLinkException e) {
			e.printStackTrace();
		}

		try {
			System.out.println("\n\n------------------------------------\nNOW reading in Memos...");
			//Now reading MEMOS
			int db2 = pl.openDB("MemoDB");
			if (db2 < 0) {
				System.out.println("ERROR! " + db2);
				System.exit(1);
			}
			for (int i = 0; i < pl.getRecordCount(db2); i++) {
				Record r = pl.getRecordByIndex(db2, i);
				MemoRecord mr = new MemoRecord(r);
				System.out.println("Memo " + i + ": " + mr.getText());
			}

			MemoRecord newMemo = new MemoRecord("The new Java-Bindings work fine");
			pl.writeRecord(db2, newMemo);

			pl.closeDB(db2);
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			System.out.println("Creating new database 'testDB2'...");
			int db3 = pl.createDB("testDB2", "RMSd", "Me");
			MemoRecord memo = new MemoRecord("new database");
			pl.writeRecord(db3, memo);
			pl.closeDB(db3);
		} catch (DatabaseExistsException e) {
			try {
				System.out.println("Database exists, deleting database 'testDB2'...");
				pl.deleteDB("testDB2");
			} catch (Exception f) {
				f.printStackTrace();
			}
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			pl.endSync();
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		pl.close();
		//done
		frame.dispose();
		System.exit(0);
	}
 

	/**
	 *  Description of the Method
	 *
	 *@param  arr  Description of the Parameter
	 */
	public static void hexdump(byte[] arr) {

		for (int i = 0; i < arr.length; ) {
			String chars = "";
			String l = "" + Integer.toHexString(i);
			while (l.length() < 4) {
				l = "0" + l;
			}
			System.out.print(l + ":  ");
			for (int j = 0; j < 16 && i < arr.length; j++, i++) {

				l = Integer.toHexString(arr[i]);
				if (arr[i] < 0) {
					l = l.substring(l.length() - 2);
				}
				while (l.length() < 2) {
					l = "0" + l;
				}
				System.out.print(l + " ");
				if ((arr[i] >= '0' && arr[i] <= 'z') || (arr[i] == ' ')) {
					chars += (char) arr[i];
				} else if (arr[i] == (byte) '\ufffd') {
					chars += "\ufffd";
				} else {
					chars += ".";
				}

			}
			System.out.println("   " + chars);
		}
	}

}

