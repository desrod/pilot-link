
import java.io.*;

public class test {

	public static void main(String[] args) throws IOException, Pdapilot.DlpException {
		System.out.println("Compiled with pilot-link " + 
			Pdapilot.constants.PILOT_LINK_VERSION + "." +
			Pdapilot.constants.PILOT_LINK_MAJOR + "." +
			Pdapilot.constants.PILOT_LINK_MINOR);
			
		Pdapilot.Socket x = new Pdapilot.Socket("/dev/cua3");
		Pdapilot.Dlp y = x.accept();

		System.out.println(y.getTime().toString());
		System.out.println(y.getUserInfo().toString());
		System.out.println(y.getSysInfo().toString());
		//System.out.println(y.getNetInfo().toString());
		
		for (int i=0;;i++) {
			Pdapilot.CardInfo c = y.getCardInfo(i);
			if (c == null)
				break;
			System.out.println("Card #" + i + ": " + c.toString() );
		}

		for (int i=0;;i++) {
			Pdapilot.DBInfo d = y.getDBInfo(i, true, true, 0);
			if (d == null)
				break;
			System.out.println("DB #" + i + ": " + d.toString() );
		}
	
		
		Pdapilot.DB z = y.open("MemoDB");
		
		Pdapilot.RecordID ids[] = z.getRecordIDs(false, 0, 0xffff);
		for (int i=0;i<ids.length;i++) {
			System.out.print(ids[i].toString()+" ");
		}
		System.out.println("");
		
		System.out.println(z.getAppBlock().toString());

		System.out.println(z.getPref(0).toString());
		
		//z.dbClass = new Pdapilot.Database(); 
		
		for (int i=0;;i++) {
			Pdapilot.Record q = (Pdapilot.Record)z.getRecord(i);
			if (q == null)
				break;
			System.out.println("Record #" + i +": " + q.toString() );
			//z.setRecord(q);
		}
		
		System.out.println("Done!");
	}
}

