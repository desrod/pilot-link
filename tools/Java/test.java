
import java.io.*;
import org.gnu.pdapilot.*;

public class test {
   public static void main( String[ ]args) {       
      System.out.println( "Compiled with pilot-link " + 	
                          Constants.PILOT_LINK_VERSION + "." + 	
                          Constants.PILOT_LINK_MAJOR + "." + 	
                          Constants.PILOT_LINK_MINOR);      
      System.out.print( "Port to use [/dev/pilot]? ");
      System.out.flush( );
      String port;
      try {
         port = org.gnu.pdapilot.Util.readLine( );
         }
      catch( Exception e) { port = ""; }
      if( port.equals( "")) port = "/dev/pilot";
      System.out.println( "Please hit the HotSync button now.");
      Dlp dlp = null;
      try {
         org.gnu.pdapilot.Socket pSock = new org.gnu.pdapilot.Socket( port);
         dlp = pSock.accept( );
         }
      catch( Exception e) {
         System.out.println( "Unable to open socket connection to " + port);
         System.exit( 1);
         }
      try {
         System.out.println( dlp.getTime( ).toString( ));
         System.out.println( dlp.getUserInfo( ).toString( ));
         System.out.println( dlp.getSysInfo( ).toString( ));
//       System.out.println( dlp.getNetInfo( ).toString( ));
         for( int i = 0; ; i++) {
            org.gnu.pdapilot.CardInfo card = dlp.getCardInfo( i);
            if( card == null) break;
            System.out.println( "Card #" + i + ": " + card.toString( ));
            }
         for (int i = 0;; i++) {
	    DBInfo dbi = dlp.getDBInfo( i, true, true, 0);
            if( dbi == null) break;
            System.out.println( "DB #" + i + ": " + dbi.toString( ));
            }
         DB db = dlp.open( new org.gnu.pdapilot.appointment.Database( ));
         RecordID ids[] = db.getRecordIDs( );
         for( int i = 0; i < ids.length; i++) {
	    System.out.print( ids[ i].toString( ) + " ");
            }
         System.out.println("");
         System.out.println( db.getAppBlock( ).toString( ));
         Pref pref;
         pref = db.getPref( 0);
         if( pref != null) System.out.println( pref.toString( ));
         pref = db.getPref( 1);
         if( pref != null) System.out.println( pref.toString( ));
         pref = db.getPref( 2);
         if( pref != null) System.out.println( pref.toString());
         pref = db.getPref( 3);
         if( pref != null) System.out.println( pref.toString());
         for( int i = 0; ; i++) {
            Record q = ( Record)db.getRecord( i);
            if( q == null) break;
            System.out.println( "Record #" + i + ": " + q.toString( ));
//          z.setRecord( q);
            }
         org.gnu.pdapilot.expense.Record newRecord = new org.gnu.pdapilot.expense.Record();
         newRecord.setPayment( org.gnu.pdapilot.expense.Payment.check);
         System.out.println( newRecord);
         db.close( );
         db = dlp.open( new org.gnu.pdapilot.memo.Database( ));
         System.out.println( db.getAppBlock( ));
         ids = db.getRecordIDs( );
         for( int i = 0; i < ids.length; i++) {
            Record q = ( Record)db.getRecord( ids[ i]);
            if( q == null) continue;
            System.out.println( "Record #" + i + ": " + q);
            }
         db.close( );         
         dlp.close( org.gnu.pdapilot.End.normal);
         System.out.println( "Done!");
         }
      catch( Exception e) {
         System.out.println( "Unexpected Exception caught");
         e.printStackTrace( );
         }
      }
   }
