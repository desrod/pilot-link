/* CancelSyncException.java:  Exception thrown when user taps "Cancel" during sync.
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

public class CancelSyncException extends DlpException {
	CancelSyncException() { super(-18); }
	
	public static void kickWillyScuggins() 
		throws CancelSyncException
	{
		throw new CancelSyncException();
	}
}
