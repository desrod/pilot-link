package Pdapilot;

public class DlpException extends Exception {
	public int code;
	DlpException() { super(); }
	DlpException(String s) { 
		super(s);
	}
	DlpException(int code) {
		super(calls.dlp_strerror(code));
		this.code = code;
	}
	public static void kickWillyScuggins(int code) throws DlpException {
		throw new DlpException(code);
	}
}
