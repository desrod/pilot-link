import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.gnu.pilotlink.MidiRecord;
import org.gnu.pilotlink.PilotLink;
import org.gnu.pilotlink.PilotLinkException;
import org.gnu.pilotlink.SysInfo;
import org.gnu.pilotlink.User;
public class MidiMgr {
	public final static int LIST=1;
	public final static int INSTALL=2;
	public final static int NOTHING=0;
	public final static int DELETE=3;
	public final static int FETCH=4;
	
	
	public static void main(String args[]) {
		String port="/dev/usb/tts/1"; //DEVFS
		String db="MIDI Ring Tones";
		int command=0;
		String arg="";
		String arg2="";
		try {
			for (int i=0; i<args.length; i++) {
				if (args[i].equals("-p")) {
					port=args[i+1];
					i++;
					continue;
				}
				
				if (args[i].equals("-l") || args[i].equals("--list")) {
					command=LIST;
					break;
				}
				if (args[i].equals("-i") || args[i].equals("--install")) {
					command=INSTALL;
					arg=args[i+1];
					if (!arg.endsWith(".mid")) {
						System.out.println("File to install must be a .mid-File!");
						System.exit(1);												
					}
					File f=new File(arg);
					if (!f.exists()) {
						System.out.println("File to install must exist! "+arg+" not found!");
						System.exit(1);
					}
					arg2=args[i+2];
					break;
				}
				if (args[i].equals("-d") || args[i].equals("--delete")) {
					command=DELETE;
					arg=args[i+1];
					break;
				}
				if (args[i].equals("-f")|| args[i].equals("--fetch")) {
					command=FETCH;
					arg=args[i+1];
					break;
				}
				
			}
		} catch (Exception e) {
			command=NOTHING;
		}
		if (command==NOTHING) {
			System.out.println("USAGE: java MidiMgr CMD ARG\nwhere CMD:\n-p PORT: Pilot-Port (default /dev/usb/tts/1)\n-db NAME: Palm-DB to use (default 'MIDI Ring Tones')\n" +
					"-l|--list : list all Midi-Files\n-d|--delete NAME: Delete File by name or id\n" +
					"-i|--install FILE NAME: Install Midi-File FILE as NAME");
			System.exit(1);
		}
		
		File p= new File(port);
		System.out.println("looking for file " + port);
		if (!p.exists()) {
			System.out.println("File does not exist... DevFS? Waiting for port to appear");
			
			while (!p.exists()) {
				System.out.print(".");
				try {
					Thread.sleep(1000);
				} catch (Exception e) {
				}
			}
		}
		PilotLink pl= null;
		try {
			pl= new PilotLink(port);
			if (!pl.isConnected()) {
				System.out.println("Something went wrong. Check output!");
				System.exit(1);
			}
		} catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
		try {
			User u= pl.getUserInfo();
			System.out.println("User: " + u.getName());
			System.out.println("Last Synchronization Date: " + u.getLastSyncDate());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			SysInfo si= pl.getSysInfo();
			System.out.println("Product ID: '" + si.getProdID() + "'");
			System.out.println("Rom Version: " + si.getRomVersion());
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		int dbh;
		try {
			int db2= pl.openDB(db);
			if (db2 < 0) {
				System.out.println("ERROR! " + db2);
				System.exit(1);
			}
			switch (command) {
			////////
			///LIST
				case LIST:
					int count=pl.getRecordCount(db2);
					for (int i=0; i<count;i++) {
						MidiRecord mr=new MidiRecord(pl.getRecordByIndex(db2,i));
						System.out.println(i+": "+mr.getId()+" "+mr.getName()+" size: "+mr.getSize());
					}
					break;
					
					//////////////////////
					////INSTALL
				case INSTALL:
					File midi=new File(arg);
					try {
						InputStream in=new FileInputStream(midi);					
						long size=midi.length();
						byte buf[]=new byte[(int)size];
						in.read(buf);						
						MidiRecord r=new MidiRecord();
						r.setName(arg2);
						r.setMidi(buf);
						in.close();
						//hexdump(r.getBuffer());
						pl.writeNewRecord(db2,r);
						System.out.println("File Installed as "+arg2);
					} catch(Exception e){
						e.printStackTrace();
					}
					break;
					
				///////////////////
					////FETCH
				case FETCH:
					
					MidiRecord mr=null;
					try {
						int id=Integer.parseInt(arg);
						mr=new MidiRecord(pl.getRecordByIndex(db2,id));
					} catch(NumberFormatException e) {
						System.out.println("No Number! Looking for file...");
						count=pl.getRecordCount(db2);
						for (int i=0; i<count;i++) {
							mr=new MidiRecord(pl.getRecordByIndex(db2,i));
							//System.out.println(i+": "+mr.getName()+" size: "+mr.getSize());
							if (mr.getName().equals(arg)) {
								break;
							}
						}
					}
					if (mr!=null) {
						File f=new File(mr.getName()+".mid");
						try {
							f.createNewFile();
							OutputStream out=new FileOutputStream(f);
							out.write(mr.getMidi());
							out.close();
							System.out.println("File "+f.getName()+" stored successfully!");
						} catch (IOException e1) {
							// TODO Auto-generated catch block
							e1.printStackTrace();
						}
						
					} else {
						System.out.println("Not found!");
					}
					break;
					
					
					////////////////
					////DELETE
				case DELETE:
					MidiRecord r=null;
					try {
						int id=Integer.parseInt(arg);
						pl.deleteRecordById(db2,id);
						System.out.println("Index "+id+" deleted.");
					} catch(NumberFormatException e) {
						System.out.println("No Number! Looking for file...");
						count=pl.getRecordCount(db2);
						for (int i=0; i<count;i++) {
							r=new MidiRecord(pl.getRecordByIndex(db2,i));
							if (r.getName().equals(arg)) {
								pl.deleteRecordById(db2,r.getId());
								break;
							}
						}
					}
					break;
			}
			
			pl.closeDB(db2);
		} catch (PilotLinkException e) {
			e.printStackTrace();
		}
		try {
			pl.endSync();
		} catch (Exception e) {
			e.printStackTrace();
		}
		pl.close();
		
		System.exit(0);
	}
	
	public static void hexdump(byte[] arr) {
		
		for (int i= 0; i < arr.length;) {
			String chars= "";
			String l= "" + Integer.toHexString(i);
			while (l.length() < 4) {
				l= "0" + l;
			}
			System.out.print(l + ":  ");
			for (int j= 0; j < 16 && i < arr.length; j++, i++) {
				
				l= Integer.toHexString(arr[i]);
				if (arr[i] < 0) {
					l= l.substring(l.length() - 2);
				}
				while (l.length() < 2) {
					l= "0" + l;
				}
				System.out.print(l + " ");
				if ((arr[i] >= '0' && arr[i] <= 'z') || (arr[i] == ' ')) {
					chars += (char) arr[i];
				} else if (arr[i] == 252) {
					chars += "\ufffd";
				} else if (arr[i] ==246) {
					chars += "\ufffd";
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
