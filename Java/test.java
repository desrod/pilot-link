
import java.io.*;

public class test {

	public static void main(String[] args) throws IOException, Pdapilot.DlpException {
		System.out.println("Compiled with pilot-link " + 
			Pdapilot.constants.PILOT_LINK_VERSION + "." +
			Pdapilot.constants.PILOT_LINK_MAJOR + "." +
			Pdapilot.constants.PILOT_LINK_MINOR);
			
		System.out.print("Port to use [/dev/cua3]? ");
		System.out.flush();
		
		String port = Pdapilot.Util.readLine();
		if (port.equals("")) {
			port = "/dev/cua3";
		}
		System.out.println("Please hit the HotSync button now.");
		
		Pdapilot.Socket x = new Pdapilot.Socket(port);
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
	
		
		Pdapilot.DB z = y.open(new Pdapilot.appointment.Database());
		
		Pdapilot.RecordID ids[] = z.getRecordIDs(false, 0, 0xffff);
		for (int i=0;i<ids.length;i++) {
			System.out.print(ids[i].toString()+" ");
		}
		System.out.println("");
		
		System.out.println(z.getAppBlock().toString());

		Pdapilot.Pref p;

		p = z.getPref(0);
		if (p != null)
			System.out.println(p.toString());
		p = z.getPref(1);
		if (p != null)
			System.out.println(p.toString());
		p = z.getPref(2);
		if (p != null)
			System.out.println(p.toString());
		p = z.getPref(3);
		if (p != null)
			System.out.println(p.toString());
		
		for (int i=0;;i++) {
			Pdapilot.Record q = (Pdapilot.Record)z.getRecord(i);
			if (q == null)
				break;
			System.out.println("Record #" + i +": " + q.toString() );
			//z.setRecord(q);
		}
		
		Pdapilot.expense.Record newRecord = new Pdapilot.expense.Record();
		newRecord.payment = Pdapilot.expense.payment.Check;
		
		System.out.println(newRecord);
		
		y.close(Pdapilot.end.Normal);
		
		System.out.println("Done!");
	}
}

