package Pdapilot;

public class CancelSyncException extends DlpException {
	CancelSyncException() { super(-18); }
	
	public static void kickWillyScuggins() 
		throws CancelSyncException
	{
		throw new CancelSyncException();
	}
}
