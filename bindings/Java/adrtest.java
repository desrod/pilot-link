import org.gnu.pilotlink.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;
/**
 *  Description of the Class
 *
 * @author     stephan
 * @created    4. Dezember 2004
 */
public class adrtest {
	/**
	 *  Description of the Method
	 *
	 * @param  args  Description of the Parameter
	 */
	public static void main(String args[]) {
		test tst=new test();
		JFrame frame = new JFrame("Progress");
		frame.getContentPane().setLayout(new BorderLayout());
		JLabel txt = new JLabel("Reading in");
		frame.getContentPane().add(txt, BorderLayout.NORTH);
		JProgressBar progress = new JProgressBar(0, 100);
		frame.getContentPane().add(progress, BorderLayout.CENTER);
		frame.setSize(300, 50);
		frame.setVisible(true);
		String port;
		if (args.length==0) {
			port="/dev/usb/tts/1";
		} else {
			port=args[0];
		}
		File p=new File(port);
		System.out.println("looking for file "+port);
		if (!p.exists()) {
			System.out.println("File does not exist... USB? Waiting for port to appear");
			
			while (!p.exists()) {
				System.out.print(".");
				try {
					Thread.sleep(1000);
				} catch (Exception e) {}
			}
		}
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
		
		try {
			SysInfo si = pl.getSysInfo();
			System.out.println("Product ID: '" + si.getProdID() + "'");
			System.out.println("Rom Version: " + si.getRomVersion());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			System.out.println("Opening DatebookDB");
			int dbhAdr=pl.openDB("AddressDB");
			System.out.println("addresses opened!");
			Record r=pl.getRecordByIndex(dbhAdr,0);
			//tst.hexdump(r.getBuffer());
			//System.out.println("Now adrrecord");
			AddressRecord adrRecord;
			/*tst.hexdump(adrRecord.getBuffer());
			System.out.println("Adding new Record...");
			AddressRecord adrRecord2=new AddressRecord();
			for (int i=0;i<19;i++) {
				if (adrRecord.getField(i)!=null) {
					adrRecord2.setField("TST:"+adrRecord.getField(i),i);
				}
			}
			
			for (int i=0;i<5;i++) {
				adrRecord2.setLabelId(adrRecord.getLabelId(i),i);
			}
			System.out.println("new Record:");
			tst.hexdump(adrRecord2.getBuffer());
			pl.writeRecord(dbhAdr,adrRecord2);
			if (true) {
				pl.endSync();
				pl.close();
				System.exit(0);
			}
			*/
			progress.setMaximum(pl.getRecordCount(dbhAdr));
			for (int i=0; i<pl.getRecordCount(dbhAdr); i++) {
				r=pl.getRecordByIndex(dbhAdr,i);
				progress.setValue(i);
				if (r==null) {
					System.out.println("Fehler beim einlesen...");
					break;
				}
				if (r.getBuffer()==null  || r.getBuffer().length==0) {
					System.out.println("0-sized record? Deleting...");
					pl.deleteRecordById(dbhAdr,r.getId());
				}
				/*if (!r.isDirty()) {
					System.out.println("entry is not dirty");
					System.out.println("Making id dirty!");
					r.setDirty(true);
					pl.writeRecord(dbhAdr,r);
				} else {
					System.out.println("Entry is dirty!");
				}*/
				adrRecord=new AddressRecord(r);
				//System.out.println("AddressRecord: "+adrRecord.getField(0)+" "+adrRecord.getField(1)+" "+adrRecord.getField(2));
				//System.out.println("Dump:");
				//tst.hexdump(adrRecord.getBuffer());
				//System.out.println();
				System.out.println("Name: "+adrRecord.getField(0)+" "+adrRecord.getField(1)+" "+adrRecord.getField(2));
				if (adrRecord.isDeleted()) {
					
					System.out.println("IS DELETED!");
				}
			}
			/*System.out.println("Deleteing Record 0!");
			adrRecord=new AddressRecord(pl.getRecordByIndex(dbhAdr,0));
			System.out.println("Adrrecord deleted: "+adrRecord.isDeleted());
			tst.hexdump(adrRecord.getBuffer());
			pl.deleteRecordById(dbhAdr,adrRecord.getId());
			*/
			
		} catch (Exception e){
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


	

}

