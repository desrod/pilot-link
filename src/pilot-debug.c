/* 
 * pilot-debug.c:  Palm debugging console, with optional graphics support
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

/* Definitions for functions in pd-tty.c */
extern void do_readline(void);

/* Display text asynchronously, and make sure it doesn't interfere with the
   current prompt */
extern void display(char * text, char * tag, int type);
 
#ifndef LIBDIR
#define LIBDIR "."
#endif

#ifdef TK
int usetk;
#include <tk.h>
#else
#include <tcl.h>
#endif

#ifndef TCL_ACTIVE
#define TCL_ACTIVE TCL_REA
#endif
#ifndef TCL_RELEASE_LEVEL
#define TCL_RELEASE_LEVEL 8
#endif

#ifdef SUNOS
/* The following variable is a special hack that is needed in order for Sun
   shared libraries to be used for Tcl. */
int *tclDummyMathPtr = (int *) matherr;
#endif

int done 	= 0;
int Interactive = 1;

Tcl_Interp * interp;        
struct Pilot_state state;

int debugger = 0; 	/* If non-zero, then debugger is thought to be
			   active */

int console = 0;  	/* If non-zero, then console is thought to be active
			   Note: if both console and debugger are thought to
			   be active, only the debugger will be usable. If
			   execution is continued, the console be be usable
			   again. */

int stalestate = 1; 	/* If non-zero, then the current Palm state (in
			   particular the registers) should be assumed
			   out-of-date */
                       
int port = 0;

int tty;		/* Non-zero means standard input is a terminal-like
			   device.  Zero means it's */


/* Misc utility */
static void SetLabel(const char * label, const char * value)
{
#ifdef TK
  if (usetk)
    Tcl_VarEval(interp, label, " configure -text "", value, """,NULL);
#endif
}

static char *itoa(int val)
{
        static 	char buf[20];
        sprintf(buf, "%d", val);
        return 	buf;
}

static char *htoa(int val)
{
        static char buf[9];
        sprintf(buf, "%8.8X", val);
        return buf;
}

static char *h4toa(int val)
{
        static char buf[5];
        sprintf(buf, "%4.4X", val);
        return buf;
}

static int SayInteractive(char *text)
{
        Tcl_DString d;

        if (!Interactive)
                return 0;

        Tcl_DStringInit(&d);

#ifdef TK
        if (usetk) {
                Tcl_DStringAppendElement(&d, ".f.t");
                Tcl_DStringAppendElement(&d, "insert");
                Tcl_DStringAppendElement(&d, "insert");
                Tcl_DStringAppendElement(&d, text);
                Tcl_Eval(interp, Tcl_DStringValue(&d));
                Tcl_DStringFree(&d);
                Tcl_DStringAppendElement(&d, ".f.t");
                Tcl_DStringAppendElement(&d, "see");
                Tcl_DStringAppendElement(&d, "insert");
                Tcl_Eval(interp, Tcl_DStringValue(&d));
                Tcl_DStringFree(&d);
        } else {
#endif
                Tcl_DStringAppendElement(&d, "puts");
                Tcl_DStringAppendElement(&d, text);
                Tcl_Eval(interp, Tcl_DStringValue(&d));
                Tcl_DStringFree(&d);

#ifdef TK
        }
#endif
        Tcl_Eval(interp, Tcl_DStringValue(&d));
        Tcl_DStringFree(&d);

        return 0;
}

/***********************************************************************
 *
 * Function:    Say
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int Say(char *text)
{
        Tcl_DString d;

        if (Interactive) {
                Tcl_DStringInit(&d);
#ifdef TK
                if (usetk) {
                        Tcl_DStringAppendElement(&d, ".f.t");
                        Tcl_DStringAppendElement(&d, "insert");
                        Tcl_DStringAppendElement(&d, "insert");
                        Tcl_DStringAppendElement(&d, text);
                        Tcl_Eval(interp, Tcl_DStringValue(&d));
                        Tcl_DStringFree(&d);
                        Tcl_DStringAppendElement(&d, ".f.t");
                        Tcl_DStringAppendElement(&d, "see");
                        Tcl_DStringAppendElement(&d, "insert");
                        Tcl_Eval(interp, Tcl_DStringValue(&d));
                        Tcl_DStringFree(&d);
                } else {
#endif
                        Tcl_DStringAppendElement(&d, "puts");
                        Tcl_DStringAppendElement(&d, text);
                        Tcl_Eval(interp, Tcl_DStringValue(&d));
                        Tcl_DStringFree(&d);

#ifdef TK
                }
#endif
        } else
                Tcl_AppendResult(interp, text, NULL);

        return 0;
}

static int Error(char *text)
{
        Tcl_SetResult(interp, text, TCL_STATIC);
        return TCL_ERROR;
}

/***********************************************************************
 *
 * Function:    SetModeLabel
 *
 * Summary:     What state are we starting up in
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void SetModeLabel(void)
{
        SetLabel(".state.halted", debugger ? "Debugger" :
                 console ? "Console" : "None");
}

/***********************************************************************
 *
 * Function:    Read_Pilot
 *
 * Summary:     Read input from the Palm. Called both via Tcl event
 *		loop on serial input, and explicitly by any function
 *		has thinks there should be a packet ready
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void
Read_Pilot(ClientData clientData, int mask)
{
   unsigned char buf[4096];
   int 	l;

   memset(buf, 0, 4096);
   l = pi_read(port, buf, 4096);

   if (l < 6)
      return;

   /* puts("From Palm:");
      dumpdata((unsigned char *)buf, l); */

   if (buf[2] == 0) {		/* SysPkt command 	*/
      if (buf[0] == 2) {	/* UI 			*/
	 if ((!console) || debugger) {
	    Say("Console active\n");
	    console 	= 1;
	    debugger 	= 0;
	    SetModeLabel();
	    Tcl_VarEval(interp, "checkin 25", NULL);
	 }
	 if (buf[4] == 0x0c) {	/* Screen update 	*/
#ifdef TK
		if (usetk) {
			int 	i,
				x1,
				y1,
				sx,
				sy,
				w,
				h,
				bytes;
			int 	y,
				x;
			char 	buffer[8192];

			Tk_PhotoImageBlock block;
			Tk_PhotoHandle handle;

			bytes 	= get_short(buf + 6);
			y1 	= get_short(buf + 8);
			x1 	= get_short(buf + 10);
			sy 	= get_short(buf + 12);
			sx 	= get_short(buf + 14);
			h 	= get_short(buf + 16);
			w 	= get_short(buf + 18);

			block.width 	= w + x1;
			block.height 	= h;
			block.pitch 	= w + x1;
			block.pixelSize = 1;
			block.offset[0] = 0;
			block.offset[1] = 0;
			block.offset[2] = 0;
			block.pixelPtr 	= buffer;

#if (TK_MAJOR_VERSION < 8) || ((TK_MAJOR_VERSION == 8) && (TK_RELEASE_LEVEL < 2))
	       handle = Tk_FindPhoto("Case");
#else
	       handle = Tk_FindPhoto(interp, "Case");
#endif
	       i 	= 0;
	       l 	= 0;
	       for (y = 0; y < h; y++) {
		  l = 20 + (y * bytes);
		  for (x = 0; x < (w + x1); x++) {
		     int mask = 1 << (7 - (x % 8));

		     buffer[i++] = (buf[x / 8 + l] & mask) ? 0 : 0xff;
		  }
	       }

	       /* for(i=0;i<((l-20)*8);i++) {
	          int p = i/8+20;
	          int b = 1<<(7-(i%8));
	          buffer[i] = (buf[p] & b) ? 0 : 0xff;
	          } */
	       Tk_PhotoPutBlock(handle, &block, 32 + sx - x1, 33 + sy - y1,
				w + x1, h);

	       Tcl_VarEval(interp, "global show; set show(.remote) 1; update",
			   NULL);
	    }
#endif
	    return;
	 }

      }
      else if (buf[0] == 1) {	/* Console 		*/
	 if (buf[4] == 0x7f) {	/* Message from Palm 	*/
	    int i;

	    for (i = 6; i < l; i++)
	       if (buf[i] == '\r')
		  buf[i] = '\n';
	    /* Insert message into both debug and console windows */
#ifdef TK
	    if (usetk) {
	       Tcl_VarEval(interp, ".console.t insert end "", buf + 6, """,
			   NULL);
	       Tcl_VarEval(interp, ".console.t mark set insert end", NULL);
	       Tcl_VarEval(interp, ".console.t see insert", NULL);
	    }
#endif
	    display(buf + 6, "Console: ", 2);

	    return;
	 }
      }
      else if (buf[0] == 0) {	/* Debug */
	 if (!debugger) {	
	    debugger = 1;
	    SetModeLabel();
	    Tcl_VarEval(interp, "checkin 25", NULL);
	 }
	 if (buf[4] == 0x7f) {	/* Message */
	    int i;

	    for (i = 6; i < l; i++)
	       if (buf[i] == '\r')
		  buf[i] = '\n';

	    display(buf + 6, "Debug: ", 1);

	    return;
	 }
	 else if (buf[4] == 0x8c) {	/* Breakpoints set */
	    Say("Breakpoint set\n");
	    return;
	 }
	 else if (buf[4] == 0x80) {	/* State response */

	    sys_UnpackState(buf + 6, &state);

	    if (stalestate) {
	       char buffer[40];

	       sprintf(buffer,
		       "Palm halted at %8.8lX (function %s) with exception %d\n",
		       state.regs.PC, state.func_name, state.exception);
	       display(buffer, "Debug: ", 1);
	       stalestate = 0;
	    }

	    SetLabel(".state.exception", itoa(state.exception));
	    SetLabel(".state.funcname", state.func_name);
	    SetLabel(".state.funcstart", htoa(state.func_start));
	    SetLabel(".state.funcend", htoa(state.func_end));
	    SetLabel(".state.d0", htoa(state.regs.D[0]));
	    SetLabel(".state.a0", htoa(state.regs.A[0]));
	    SetLabel(".state.d1", htoa(state.regs.D[1]));
	    SetLabel(".state.a1", htoa(state.regs.A[1]));
	    SetLabel(".state.d2", htoa(state.regs.D[2]));
	    SetLabel(".state.a2", htoa(state.regs.A[2]));
	    SetLabel(".state.d3", htoa(state.regs.D[3]));
	    SetLabel(".state.a3", htoa(state.regs.A[3]));
	    SetLabel(".state.d4", htoa(state.regs.D[4]));
	    SetLabel(".state.a4", htoa(state.regs.A[4]));
	    SetLabel(".state.d5", htoa(state.regs.D[5]));
	    SetLabel(".state.a5", htoa(state.regs.A[5]));
	    SetLabel(".state.d6", htoa(state.regs.D[6]));
	    SetLabel(".state.a6", htoa(state.regs.A[6]));
	    SetLabel(".state.d7", htoa(state.regs.D[7]));

	    SetLabel(".state.ssp", htoa(state.regs.SSP));
	    SetLabel(".state.usp", htoa(state.regs.USP));
	    SetLabel(".state.sr", h4toa(state.regs.SR));
	    SetLabel(".state.pc", htoa(state.regs.PC));

	    SetLabel(".state.reset", state.reset ? "Yes" : "No");

	    SetLabel(".state.b1", htoa(state.breakpoint[0].address));
	    SetLabel(".state.b1a",
		     (state.breakpoint[0].enabled) ? "Yes" : "No");
	    SetLabel(".state.b2", htoa(state.breakpoint[1].address));
	    SetLabel(".state.b2a",
		     (state.breakpoint[1].enabled) ? "Yes" : "No");
	    SetLabel(".state.b3", htoa(state.breakpoint[2].address));
	    SetLabel(".state.b3a",
		     (state.breakpoint[2].enabled) ? "Yes" : "No");
	    SetLabel(".state.b4", htoa(state.breakpoint[3].address));
	    SetLabel(".state.b4a",
		     (state.breakpoint[3].enabled) ? "Yes" : "No");
	    SetLabel(".state.b5", htoa(state.breakpoint[4].address));
	    SetLabel(".state.b5a",
		     (state.breakpoint[4].enabled) ? "Yes" : "No");
	    SetLabel(".state.b6", htoa(state.breakpoint[5].address));
	    SetLabel(".state.b6a",
		     (state.breakpoint[5].enabled) ? "Yes" : "No");

	    /* Show the state window if it is hidden */
	    Tcl_VarEval(interp, "set show(.state) 1", NULL);
	    return;
	 }
      }
   }
}

/***********************************************************************
 *
 * Function:    ParseAddress
 *
 * Summary:     Turn an address string into a numeric value. Ideally, 
 *		it should also know about traps, function names, etc. 
 *		As it is, it just assumed the text is a hexadecimal
 * 		number, with or with '0x' prefix.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static unsigned long ParseAddress(char *address)
{
        return strtoul(address, 0, 16);
}

/***********************************************************************
 *
 * Function:    SetBreakpoint
 *
 * Summary:     Utility function to modify breakpoint table
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int SetBreakpoint(int bp, unsigned long address, int enabled)
{

        state.breakpoint[bp].address = address;
        state.breakpoint[bp].enabled = enabled;

        SetLabel(".state.b1", htoa(state.breakpoint[0].address));
        SetLabel(".state.b1a",
                 (state.breakpoint[0].enabled) ? "Yes" : "No");
        SetLabel(".state.b2", htoa(state.breakpoint[1].address));
        SetLabel(".state.b2a",
                 (state.breakpoint[1].enabled) ? "Yes" : "No");
        SetLabel(".state.b3", htoa(state.breakpoint[2].address));
        SetLabel(".state.b3a",
                 (state.breakpoint[2].enabled) ? "Yes" : "No");
        SetLabel(".state.b4", htoa(state.breakpoint[3].address));
        SetLabel(".state.b4a",
                 (state.breakpoint[3].enabled) ? "Yes" : "No");
        SetLabel(".state.b5", htoa(state.breakpoint[4].address));
        SetLabel(".state.b5a",
                 (state.breakpoint[4].enabled) ? "Yes" : "No");
        SetLabel(".state.b6", htoa(state.breakpoint[5].address));
        SetLabel(".state.b6a",
                 (state.breakpoint[5].enabled) ? "Yes" : "No");

        sys_SetBreakpoints(port, &state.breakpoint[0]);

        Read_Pilot(0, 0);

        return 0;
}

int majorVersion, minorVersion, stateVersion, buildVersion;

/***********************************************************************
 *
 * Function:    DbgGetDeviceVersion
 *
 * Summary:     Return the device version in use
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void DbgGetDeviceVersion(void)
{
        int 	result;
        struct 	RPC_params p;
        unsigned long ROMversion;

        ROMversion = 0x12345678;

        PackRPC(&p, 0xA27B, RPC_IntReply,
                RPC_Long(makelong("psys")), RPC_Short(1),
                RPC_LongPtr(&ROMversion), RPC_End);

        DoRPC(port, 1, &p, &result);

        majorVersion =
            (((ROMversion >> 28) & 0xf) * 10) + ((ROMversion >> 24) & 0xf);
        minorVersion =
            (((ROMversion >> 20) & 0xf) * 10) + ((ROMversion >> 16) & 0xf);
        stateVersion = ((ROMversion >> 12) & 0xf);
        buildVersion =
            (((ROMversion >> 8) & 0xf) * 10) +
            (((ROMversion >> 4) & 0xf) * 10) + (ROMversion & 0xf);
}
                              
/***********************************************************************
 *
 * Function:    DbgAttach
 *
 * Summary:     Attempt to verify a connection to either the debugger
 *		or console
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int DbgAttach(int verify)
{
        struct 	RPC_params p;

        if (!port) {
                Error
                    ("No serial port selected, use 'port' command to choose one.\n");
                return 0;
        }

      again:
        if (verify || (!debugger && !console)) {
                int old = debugger;

                sys_QueryState(port);
                debugger = 0;
                Read_Pilot(0, 0);
                if (debugger && !old) {
                        if (verify > 1)
                                Say("Attaching to Palm debugger\n");
                        else
                                SayInteractive
                                    ("(attaching to Palm debugger)\n");

                }
        }

        if (!debugger && (verify || !console)) {
                int 	err,
			old = console;

                console = 0;
                PackRPC(&p, 0xA09E, RPC_IntReply, RPC_End);     /* TaskID, a harmless call */
                DoRPC(port, 1, &p, &err);
                if (err == 0)
                        console = 1;
                else
                        console = 0;
                if (console && !old) {
                        if (verify > 1)
                                Say("Attaching to Palm console\n");
                        else
                                SayInteractive
                                    ("(attaching to Palm console)\n");
                }
        }

        if (!debugger && !console && !verify) {
                verify = 1;
                goto again;
        }

        SetModeLabel();

        return (debugger || console);
}

/***********************************************************************
 *
 * Function:    DbgAttachDebugger
 *
 * Summary:     Bind the debugger to the open socket and port
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int DbgAttachDebugger(int verify)
{
        if (!port) {
                Error
                    ("No serial port selected, use 'port' command to choose one.\n");
                return 0;
        }

        if (!debugger || verify) {
                int 	old 	= debugger;
                debugger 	= 0;
		
                sys_QueryState(port);
                Read_Pilot(0, 0);
                SetModeLabel();
                if (debugger && !old)
                        SayInteractive("(attaching to Palm debugger)\n");
        }

        if (!debugger) {
                Error
                    ("Unable to attach to debugger on Palm. Is the Palm connected and in debugging mode?\n");
        }
        return debugger;
}

/***********************************************************************
 *
 * Function:    DbgAttachConsole
 *
 * Summary:     Attach an interactive debugging console to the socket
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int DbgAttachConsole(int verify)
{
        int 	err;
        struct 	RPC_params p;

        if (!port) {
                Error
                    ("No serial port selected, use 'port' command to choose one.\n");
                return 0;
        }

        if (verify || debugger || !console) {
                int old = console && !debugger;

                PackRPC(&p, 0xA09E, RPC_IntReply, RPC_End);     /* TaskID, a harmless call */
                DoRPC(port, 1, &p, &err);
                if (err == 0) {
                        console 	= 1;
                        debugger 	= 0;
                        SetModeLabel();
                } else {
                        if (!debugger)
                                console = 0;
                }
                if ((console && !debugger) && !old)
                        SayInteractive("(attaching to Palm console)\n");
        }

        if (!console || debugger) {
                Error
                    ("Unable to attach to console on Palm. Is the Palm connected, in console mode, and not in debugger mode?\n");
        }

        return console;
}

/***********************************************************************
 *
 * Function:    DbgRPC
 *
 * Summary:     Low-level debugging routines via RPC to the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static unsigned long DbgRPC(struct RPC_params *p, int *error)
{
        unsigned long result = 0;
        int 	err = -1;

        if (!port) {
                Error
                    ("No serial port selected, use 'port' command to choose one.\n");
                return 0;
        }

        if (p->reply == RPC_NoReply) {
                /* If the RPC call will normally generate no reply (as in a call that
                   reboots the machine) then we need to do some special work to verify
                   that the connection is active */
                DbgAttach(1);
                if (!debugger && !console) {
                        if (error)
                                *error = -1;
                        Error
                            ("Unable to invoke RPC on Palm. Is the Palm connected and in debugging or console mode?\n");
                        return 0;
                }
        }

        if (debugger) {
                result = DoRPC(port, 0, p, &err);
                if ((err < 0) && (p->reply != RPC_NoReply)) {   /* Failure, assume no response */
                        debugger = 0;
                        SetModeLabel();
                }
        } else if (console) {
                result = DoRPC(port, 1, p, &err);
                if ((err < 0) && (p->reply != RPC_NoReply)) {   /* Failure, assume no response */
                        console = 0;
                        SetModeLabel();
                }
        }
        /* else {
           Say("(attaching to Palm)\n");
           } */
        if (!console && !debugger) {
                DbgAttach(0);
                if (debugger) {
                        result = DoRPC(port, 0, p, &err);
                } else if (console) {
                        result = DoRPC(port, 1, p, &err);
                } else {
                        /* complete failure to attach */
                        Error
                            ("Unable to invoke RPC on Palm. Is the Palm connected and in debugging or console mode?\n");
                }
        }

        if (error)
                *error = err;

        return result;
}

unsigned char buffer[0xffff];
/***********************************************************************
 *
 * Function:    proc_g
 *
 * Summary:     Go, restart execution
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_g(ClientData clientData, Tcl_Interp * interp, int argc,
		  char *argv[])
{
        /* Use verify since the sys_Continue command produces no return value */

        if (!DbgAttachDebugger(1))
                return TCL_ERROR;

        if (argc == 2) {
                /* argv[1] is address to start execution at */
                state.regs.PC = ParseAddress(argv[1]);
                SetLabel(".state.pc", htoa(state.regs.PC));

                /*SetBreakpoint(port, 5, ParseAddress(argv[1]), 1); */
        }

        sys_Continue(port, &state.regs, 0);

        Say("Resuming execution\n");

        /* Assume the Palm is no longer halted */
        debugger 	= 0;
        console 	= 0;
        stalestate 	= 1;
        SetModeLabel();

        Tcl_VarEval(interp, "checkupin 25", NULL);
        Tcl_ResetResult(interp);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_t
 *
 * Summary:     Till, restart execution
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_t(ClientData clientData, Tcl_Interp * interp, int argc,
		  char *argv[])
{
        /* Use verify since the sys_Continue command produces no return value */

        if (!DbgAttachDebugger(1))
                return TCL_ERROR;

        if (argc >= 2) {
                /* argv[1] is address to stop execution at */
                SetBreakpoint(5, ParseAddress(argv[1]), 1);
        }

        if (argc >= 3) {
                /* argv[2] is address to start execution at */
                state.regs.PC = ParseAddress(argv[2]);
                SetLabel(".state.pc", htoa(state.regs.PC));
        }

        sys_Continue(port, &state.regs, 0);

        Say("Resuming execution\n");

        /* Assume the Palm is no longer halted */
        debugger 	= 0;
        console 	= 0;
        stalestate 	= 1;
        SetModeLabel();

        Tcl_VarEval(interp, "checkupin 25", NULL);
        Tcl_ResetResult(interp);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_attach
 *
 * Summary:     Attach to a Palm that has already crashed into the
 *		debugger without notifying us
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_attach(ClientData clientData, Tcl_Interp * interp, int argc,
		       char *argv[])
{
        DbgAttach(2);           /* Two means explicit verify, as opposed to implicit */

        if (!debugger && !console) {
                Say("Unable to attach to to Palm. Is the Palm connected and in debugging or console mode?\n");
        }

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_sendscreen
 *
 * Summary:     Display the screen output in the GUI window
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_sendscreen(ClientData clientData, Tcl_Interp * interp, int argc,
			   char *argv[])
{
        struct RPC_params p;

        PackRPC(&p, 0xA0F1, RPC_IntReply, RPC_Short(0), RPC_Short(0),
                RPC_Short(160), RPC_Short(160), RPC_End);
        DbgRPC(&p, 0);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_coldboot
 *
 * Summary:     Drop all communication, restart device
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_coldboot(ClientData clientData, Tcl_Interp * interp, int argc,
			 char *argv[])
{
        struct RPC_params p;

        PackRPC(&p, 0xA08B, RPC_NoReply, RPC_Long(0), RPC_Long(0),
                RPC_Long(0), RPC_Long(0), RPC_End);
        DbgRPC(&p, 0);

        /* And sever attachment */
        debugger 	= 0;
        console 	= 0;
        stalestate 	= 1;
        SetModeLabel();

        Tcl_VarEval(interp, "checkupin 25", NULL);
        Tcl_ResetResult(interp);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_warmboot
 *
 * Summary:     Simple "pin reset" restart
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_warmboot(ClientData clientData, Tcl_Interp * interp, int argc,
			 char *argv[])
{
        struct RPC_params p;

        PackRPC(&p, 0xA08C, RPC_NoReply, RPC_End);
        DbgRPC(&p, 0);

        /* And sever attachment */
        debugger 	= 0;
        console 	= 0;
        stalestate 	= 1;
        SetModeLabel();

        Tcl_VarEval(interp, "checkupin 25", NULL);
        Tcl_ResetResult(interp);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_battery
 *
 * Summary:     Display and query battery status information
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_battery(ClientData clientData, Tcl_Interp * interp, int argc,
			char *argv[])
{
        int 	err,
		warn,
		critical,
		maxTicks,
		v,
		kind,
		pluggedin;
        char 	buffer[30];
        struct 	RPC_params p;

        if (!DbgAttach(0))
                return TCL_ERROR;

        warn 		= 0x1234;
        critical 	= 0x2345;
        maxTicks 	= 0x3456;
        kind 		= 2;
        pluggedin 	= 3;

        PackRPC(&p, 0xA0B6, RPC_IntReply,
                RPC_Byte(0), RPC_ShortPtr(&warn), RPC_ShortPtr(&critical),
                RPC_ShortPtr(&maxTicks), RPC_BytePtr(&kind),
                RPC_BytePtr(&pluggedin), RPC_End);

        v = DbgRPC(&p, &err);

        if (err)
                return TCL_ERROR;

/* printf("Volts = %f, crit = %f, warn = %f, ticks = %d, kind =
   %d, pluggedin= %d, err = %d\n", (float)v/100, (float)critical/100,
   (float)warn/100, maxTicks, kind, pluggedin, err); */

        sprintf(buffer, "%.2f %s%s", (float) v / 100,
                (kind == 0) ? "Alkaline" : (kind == 1) ? "NiCd" : (kind ==
                                                                   2) ?
                "Lithium" : "", pluggedin ? " Ext" : "");

        Say(buffer);

        SetLabel(".state.battery", buffer);

#ifdef TK
        if (usetk)
                Tcl_VarEval(interp, ".state.battery configure -fg ",
                            (v <= critical) ? "red" : (v <=
                                                       warn) ? "darkred" :
                            "blue", NULL);
#endif

        Say((v <= critical) ? " power critical!\n" : (v <=
                                                      warn) ?
            " power low\n" : "\n");

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_mirror
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_mirror(ClientData clientData, Tcl_Interp * interp, int argc,
		       char *argv[])
{
        int 	e1,
		e2,
		active,
		scrGlobals,
		doDrawNotify;
        struct 	RPC_params p;
        unsigned long addr;

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        DbgGetDeviceVersion();

        switch (majorVersion) {
          case 1:
                  scrGlobals 	= 356;
                  doDrawNotify 	= 18;
                  break;
          case 2:
                  scrGlobals 	= 356;
                  doDrawNotify 	= 18;
                  break;
          default:
                  Say("I don't know how to change mirroring on this device version\n");
                  return TCL_ERROR;
        }

        /* Fetch scrGlobals ptr */
        PackRPC(&p, 0xA026, RPC_IntReply, RPC_LongPtr(&addr),
                RPC_Long(scrGlobals), RPC_Long(4), RPC_End);
        e1 = DbgRPC(&p, &e2);

        /* Fetch current drawnotify setting */
        PackRPC(&p, 0xA026, RPC_IntReply, RPC_BytePtr(&active),
                RPC_Long(addr + doDrawNotify), RPC_Long(1), RPC_End);
        e1 = DbgRPC(&p, &e2);

        /*printf("addr=%d\nactive=%d\n", addr, active); */

        /* Change setting */
        if (argc < 2)
                active = !active;
        else if (Tcl_GetBoolean(interp, argv[1], &active) != TCL_OK)
                return TCL_ERROR;

        /* Put it back */
        PackRPC(&p, 0xA026, RPC_IntReply, RPC_Long(addr + doDrawNotify),
                RPC_BytePtr(&active), RPC_Long(1), RPC_End);
        e1 = DbgRPC(&p, &e2);

        if (Interactive) {
                if (active)
                        Say("Mirroring is active.\n");
                else
                        Say("Mirroring is inactive.\n");
        }

        if (active) {
                /* Try to prod Palm into immediately giving us a screen update */
                PackRPC(&p, 0xA0F1, RPC_IntReply, RPC_Short(0),
                        RPC_Short(0), RPC_Short(160), RPC_Short(160),
                        RPC_End);
                e1 = DbgRPC(&p, &e2);
        }

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_updatedisplay
 *
 * Summary:     Force a screen update from the Palm
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_updatedisplay(ClientData clientData, Tcl_Interp * interp,
			      int argc, char *argv[])
{
        int 	e1,
		e2;
        struct 	RPC_params p;

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        /* Try to prod Palm into immediately giving us a screen update */
        PackRPC(&p, 0xA0F1, RPC_IntReply, RPC_Short(0), RPC_Short(0),
                RPC_Short(160), RPC_Short(160), RPC_End);
        e1 = DbgRPC(&p, &e2);

        return TCL_OK;
}

#define DB 0xFFFF0000
#define LSSA 0xFA00  

static int proc_getdisplay(ClientData clientData, Tcl_Interp * interp, int argc,
			   char *argv[])
{
#ifndef TK
        Say("getdisplay is not available due to pilot-debug being compiled without Tk support");
        return TCL_ERROR;

#else
        int 	e1,
		e2,
		l;
        char 	buffer[0xffff],
		buffer2[0xffff];
        Tk_PhotoImageBlock block;
        Tk_PhotoHandle handle;
        struct 	RPC_params p;
        unsigned long addr;

        if (!usetk) {
                Say("getdisplay is not usable when graphical display is disabled");
                return TCL_ERROR;
        }

        if (!DbgAttach(0))
                return TCL_ERROR;

        if (debugger) {
                l = sys_ReadMemory(port, DB + LSSA, 4, buffer);
                addr = get_long(buffer);
                l = sys_ReadMemory(port, addr, 160 * 160 / 8, buffer);
        } else {
                PackRPC(&p, 0xA026, RPC_IntReply, RPC_LongPtr(&addr),
                        RPC_Long(DB + LSSA), RPC_Long(4), RPC_End);
                e1 = DbgRPC(&p, &e2);
                for (l = 0; l < 160 * 160 / 8; l += 64) {
                        PackRPC(&p, 0xA026, RPC_IntReply,
                                RPC_Ptr(buffer + l, 128),
                                RPC_Long(addr + l), RPC_Long(128),
                                RPC_End);
                        e1 = DbgRPC(&p, &e2);
                }
        }

        block.width 	= 160;
        block.height 	= 160;
        block.pitch 	= 160;
        block.pixelSize = 1;
        block.offset[0] = 0;
        block.offset[1] = 0;
        block.offset[2] = 0;

#if (TK_MAJOR_VERSION < 8) || ((TK_MAJOR_VERSION == 8) && (TK_RELEASE_LEVEL < 2))
        handle = Tk_FindPhoto("Case");
#else
        handle = Tk_FindPhoto(interp, "Case");
#endif

        for (l = 0; l < 160 * 160; l++) {
                int p = l / 8;
                int b = 1 << (7 - (l % 8));

                buffer2[l] = (buffer[p] & b) ? 0 : 0xff;
        }
        block.pixelPtr = buffer2;
        Tk_PhotoPutBlock(handle, &block, 32, 33, 160, 160);

        return TCL_OK;
#endif
}

/***********************************************************************
 *
 * Function:    proc_transmit
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_transmit(ClientData clientData, Tcl_Interp * interp, int argc,
			 char *argv[])
{
        if (argc < 2)
                return TCL_OK;

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        buffer[0] = 1;
        buffer[1] = 1;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0x7f;
        buffer[5] = 0;
        strcpy(buffer + 6, argv[1]);
        strcat(buffer + 6, "\n");

        pi_write(port, buffer, 6 + strlen(argv[1]) + 2);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_pushbutton
 *
 * Summary:     Simulate and emulate the button press actions
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_pushbutton(ClientData clientData, Tcl_Interp * interp, int argc,
			   char *argv[])
{
        struct 	RPC_params p;
        unsigned int key 	= 0;
	unsigned int scan 	= 0;
	unsigned int mod 	= 0;

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        sys_RPCerror = 0;
        switch (atoi(argv[1])) {
          case 0:               /* Backlight */
                  key = 0x0113;
                  mod = 0x08;
                  break;
          case 1:               /* Power */
                  key = 0x0208;
                  mod = 0x08;
                  break;
          case 2:               /* Datebook */
                  key = 0x0204;
                  mod = 0x08;
                  break;
          case 3:               /* Address */
                  key = 0x0205;
                  mod = 0x08;
                  break;
          case 6:               /* ToDo */
                  key = 0x0206;
                  mod = 0x08;
                  break;
          case 7:               /* Memo */
                  key = 0x0207;
                  mod = 0x08;
                  break;
          case 4:               /* Page Up */
                  key = 0x000b;
                  mod = 0;
                  break;
          case 5:               /* Page Down */
                  key = 0x000c;
                  mod = 0;
                  break;
          default:
                  return Error("Button number out of range (1-7)");
        }

        PackRPC(&p, 0xA12D, RPC_IntReply, RPC_Short(key), RPC_Short(scan),
                RPC_Short(mod), RPC_End);

        DbgRPC(&p, 0);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_pen
 *
 * Summary:     Tcl procedure to simulate a pen tap -- primarily used
 *		by Remote UI bindings
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_pen(ClientData clientData, Tcl_Interp * interp, int argc,
		    char *argv[])
{
        int x = atoi(argv[1]) - 32, y = atoi(argv[2]) - 33, pen =
            atoi(argv[3]);

        /* Transmit Pen event to Palm */

        /*printf("Pen %d, %d, %d\n", x, y, pen); */

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        sys_RemoteEvent(port, pen, x, y, 0, 0, 0, 0);

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_key
 *
 * Summary:     Tcl procedure to simulate a key press -- primarily used
 *		by Remote UI bindings
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_key(ClientData clientData, Tcl_Interp * interp, int argc,
		    char *argv[])
{
        /*struct RPC_params p; */
        int 	key = argv[1][0];

        /* Change \r to \n */
        if (key == 13)
                key = 10;

        /* Transmit ASCII key to Palm */

        /*printf("Key %d\n", key); */

        /*PackRPC(&p, 0xA12D, RPC_IntReply, RPC_Short(key),RPC_Short(0),RPC_Short(0x0), RPC_End);

           if (key != 0)
           DbgRPC(&p, 0); */

        if (!DbgAttachConsole(0))
                return TCL_ERROR;

        sys_RemoteEvent(port, 0, 0, 0, 1, 0, key, 0);

/*  int sd, int penDown, int x, int y, int keypressed,
                         int keymod, int keyasc, int keycode)*/

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_port
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_port(ClientData clientData, Tcl_Interp * interp, int argc,
		     char *argv[])
{
        static 	struct pi_sockaddr laddr;
        static 	Tcl_Channel channel;
        int 	fd;

        if (argc < 2) {
                if (port)
                        Tcl_SetResult(interp, laddr.pi_device, TCL_STATIC);
                return TCL_OK;
        }

        if (port) {
                Tcl_DeleteChannelHandler(channel, Read_Pilot,
                                         (ClientData) port);
                Tcl_UnregisterChannel(interp, channel);
                pi_close(port);
                port = 0;
        }

        if (strcmp(argv[1], "close") == 0) {
                return TCL_OK;
        }

/*        port = pi_socket(PI_AF_SLP, PI_SOCK_RAW, PI_PF_SLP); */

/*       laddr.pi_family = PI_AF_SLP; */
        strcpy(laddr.pi_device, argv[1]);

        if (pi_bind(port, (struct sockaddr *) &laddr, sizeof(laddr)) == -1) {
                Say("Unable to open port '");
                Say(argv[1]);
                Say("': ");
                Say(Tcl_ErrnoMsg(errno));
                Say("\n");
                port = 0;
                return TCL_ERROR;
        }

        fd = port;

#if (TCL_MAJOR_VERSION<8 || (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION <3))
        channel = Tcl_MakeFileChannel((ClientData) fd, 0, TCL_READABLE);
#else
        channel = Tcl_MakeFileChannel((ClientData) fd, TCL_READABLE);
#endif
        Tcl_RegisterChannel(interp, channel);   /* And register it to TCL */
        Tcl_CreateChannelHandler(channel, TCL_READABLE, Read_Pilot,
                                 (ClientData) port);

        Tcl_SetResult(interp, laddr.pi_device, TCL_STATIC);
        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_feature
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_feature(ClientData clientData, Tcl_Interp * interp, int argc,
			char *argv[])
{

        if (argc < 2) {
                Tcl_SetResult(interp,
                              "feature: -all|-unreg <type> <id>|-set <type> <id> <value>|-get <type> <id>",
                              TCL_STATIC);
                return TCL_OK;
        }

        if (!DbgAttach(0))
                return TCL_ERROR;

        if (strcmp(argv[1], "-all") == 0) {
                struct RPC_params p;
                unsigned long type, value;
                unsigned short int id_;
                int i, j;

                for (j = 0; j < 2; j++) {       /* 0: RAM, 1: ROM */
                        Say(j ? "ROM:\n" : "RAM:\n");
                        for (i = 0;; i++) {
                                unsigned long e1 = 0;
                                int e2 = 0;
                                char buffer[40];

                                PackRPC(&p, 0xA27D, RPC_IntReply,
                                        RPC_Short(i), RPC_Byte(j),
                                        RPC_LongPtr(&type),
                                        RPC_ShortPtr(&id_),
                                        RPC_LongPtr(&value), RPC_End);
                                e1 = DbgRPC(&p, &e2);
                                if (e1 || e2)
                                        break;
                                sprintf(buffer,
                                        "\t%s, 0x%4.4x (%d) = 0x%8.8lx (%lu)",
                                        printlong(type), id_, id_, value,
                                        value);
                                Say(buffer);
                        }
                }
        } else if (strcmp(argv[1], "-unreg") == 0) {
                struct RPC_params p;
                unsigned long type, e1;
                unsigned short int id_;
                int e2;

                type = makelong(argv[2]);
                id_ = atoi(argv[3]);

                PackRPC(&p, 0xA27A, RPC_IntReply, RPC_Long(type),
                        RPC_Short(id_), RPC_End);

                e1 = DbgRPC(&p, &e2);
        }

        return TCL_OK;
}

/***********************************************************************
 *
 * Function:    proc_inittkdbg
 *
 * Summary:     Build the GUI and import all the usable elements
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int proc_inittkdbg(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
#ifndef TK
  Say("This executable does not contain Tk support!\n");
  return TCL_ERROR;
#else
  static int created = 0;
  if (created) {
    Say("Graphical debugger already initialized.\n");
    return TCL_OK;
  }
  Tcl_EvalFile(interp, "proc_inittkdbg.tcl");
    return TCL_OK;
#endif
}

static int proc_help(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  Tcl_EvalFile(interp, "myfunc.tcl");
  return TCL_OK;
}

struct { char * name; Tcl_CmdProc * proc; } cmds[] = {
	{ "coldboot",        proc_coldboot },
	{ "warmboot",        proc_warmboot },
	{ "sendscreen",      proc_sendscreen },
	{ "pushbutton",      proc_pushbutton },
	{ "pen",             proc_pen },
	{ "key",             proc_key },
	{ "g",               proc_g },
	{ "t",               proc_t },
	{ "attach",          proc_attach },
	{ "transmit",        proc_transmit },
	{ "getdisplay",      proc_getdisplay },
	{ "updatedisplay",   proc_updatedisplay },
	{ "mirror",          proc_mirror },
	{ "battery",         proc_battery },
	{ "port",            proc_port },
	{ "help",            proc_help },
	{ "inittkdbg",       proc_inittkdbg },
	{ "feature",         proc_feature },
	{ 0, 0}
};

int
Tcl_AppInit(myinterp)
    Tcl_Interp *myinterp;		/* Interpreter for application. */
{
    int i;

    interp=myinterp;

    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
#ifdef TK
    if (usetk) {
        if (Tk_Init(interp) == TCL_ERROR) {
          return TCL_ERROR;
        }
        Tcl_StaticPackage(interp, "Tk", Tk_Init, (Tcl_PackageInitProc *) NULL);
    }
#endif

  /*** Load custom Tcl procedures ***/


  for (i=0;cmds[i].name;i++) {
    Tcl_CreateCommand(interp, cmds[i].name, cmds[i].proc, 0, NULL);
  }

  Tcl_VarEval(interp,"\
proc Say {text} {\
	global Interactive\
	if {$Interactive} {\
	  puts $text\
	} else {\
	  upvar result result\
	  set result $result$text\
	}\
}",NULL);     

  Tcl_LinkVar(interp, "Interactive", (char*)&Interactive, TCL_LINK_INT);


#ifdef TK  
  if (usetk)
    Tcl_VarEval(interp,"inittkdbg",NULL);
  else
    Tcl_VarEval(interp,"set tkdbg 0", NULL);
#endif

static char welcomeScript[] = 
	"Welcome to pilot-debug!\n\n"
	"\tType 'help' for further information.\n"
	"\tType 'exit' to quit the application\n\n";

Say(welcomeScript);

#if 0
static char welcomeScriptDebug[] = 
	"\tWelcome to pilot-debug!\n\n"

	"Please connect your Palm and start console or debugging mode.\n\n"

	"(Console mode is a background task that can respond to a few\n"
	"commands, most importantly RPC which lets any function on the Palm\n"
	"be invoked. The Palm operates as usual while console mode is active,\n"
	"except that since the serial port is held open, HotSync and other\n"
	"applications that use the serial port will not work. Debug mode is\n"
	"activated on demand or when the Palm crashes. In debug mode, the CPU\n"
	"is halted, and no commands may be executed, except via a debugging\n"
	"console like this one.)\n\n"

	"In the absence of special utilities, the console can be started by\n"
	"the ".2\" shortcut, and debugging via \".1\". To clear either mode,\n"
	"reboot via the reset button. If console mode is active, you may also\n"
	"reboot via the "coldboot" or "warmboot" commands.\n\n"

	"The Remote UI window lets you manipulate the Palm if console mode is\n"
	"active. By clicking the mouse button on the screen or buttons, you\n"
	"can simulate pen taps, and if you type anything while the window has\n"
	"the focus, the Palm will receive the keystrokes.\n\n"

	"The Remote Console window is specifically for the transmission and\n"
	"reception of console packets. Pressing Return on a line will\n"
	"transmit it, and any incoming packets will be displayed here in\n"
	"addition to the Debug Console.\n\n"

	"The Remote State window shows the current Palm CPU state. It is only\n"
	"updated on request or when the Palm halts.\n\n"

	"The Debugging Console window is the primary interface for\n"
	"pilot-debug.  Pressing Return on a line that contains text will\n"
	"execute that line as a Tcl command. (Try 'expr 3+4'.) All of the\n"
	"usual Tcl and Tk commands are available, as well as some\n"
	"special-purpose ones, including 'help', 'coldboot', 'warmboot',\n"
	"'attach', 't', and 'g', (the last one continues after the Palm\n"
	"halts.)\n\n"

	"Execute 'help' for the list of commands currently implemented.\n\n";

Say(welcomeScriptDebug);

#endif

    /* Specify a user-specific startup file to invoke if the application is
       run interactively.  Typically the startup file is "~/.apprc" where
       "app" is the name of the application.  If this line is deleted then
       no user-specific startup file will be run under any conditions. */

    Tcl_SetVar(interp, "tcl_rcFileName", "~/.pdebugrc", TCL_GLOBAL_ONLY);


	static char debugPrompt[] = 
		"set tcl_prompt1 myprompt"
		"proc myprompt {} {"
		"puts -nonewline \"pilot-debug> \""
		"}";
      
   Tcl_VarEval(interp, debugPrompt, NULL);

    /* Deal with command-line arguments */
	static char commandArgs[] =
		"if {$argc > 0} {"
			"set p [lindex $argv 0]"
			"set argv [lrange $argv 1 end]"
			"port $p"
		"} else {"
			"Say \"As you have not entered a serial port on the command like, you might like to"
			"set one with 'port /dev/something'\n\n\""
		"}";

   Tcl_VarEval(interp, commandArgs, NULL);
        
    return TCL_OK;
}

int main(int argc, char *argv[])
{
	int 	code,
		exitCode = 0;

	char 	*args, *fileName,
		buf[20];

	size_t length;

	Tcl_Channel errChannel;

	Tcl_FindExecutable(argv[0]);
	interp = Tcl_CreateInterp();

	fileName = NULL;
	if (argc > 2) {
		length = strlen(argv[1]);
		if ((length >= 2)
		    && (strncmp(argv[1], "-file", length) == 0)) {
			fileName = argv[2];
			argc -= 2;
			argv += 2;
		}
	}
#ifdef TK
	usetk = 1;
#endif

	if (argc > 1) {
		length = strlen(argv[1]);
		if ((length >= 2)
		    && (strncmp(argv[1], "-notk", length) == 0)) {
#ifdef TK
			usetk = 0;
#endif
			argc--;
			argv++;
		}
	}

	/* Make command-line arguments available in the Tcl variables "argc" and
	   "argv". */

	args = Tcl_Merge(argc - 1, argv + 1);
	Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
	ckfree(args);
	sprintf(buf, "%d", argc - 1);
	Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "argv0",
		   (fileName != NULL) ? fileName : argv[0],
		   TCL_GLOBAL_ONLY);

	/* Set the "tcl_interactive" variable. */

	tty = isatty(0);
	Tcl_SetVar(interp, "tcl_interactive",
		   ((fileName == NULL)
		    && tty) ? "1" : "0", TCL_GLOBAL_ONLY);

#ifdef TK
	if (!getenv("DISPLAY") || (strlen(getenv("DISPLAY")) == 0))
		usetk = 0;
#endif

	/* Invoke application-specific initialization. */

	if (Tcl_AppInit(interp) != TCL_OK) {
		errChannel = Tcl_GetStdChannel(TCL_STDERR);
		if (errChannel) {
			Tcl_Write(errChannel,
				  "application-specific initialization failed: ",
				  -1);
			Tcl_Write(errChannel, interp->result, -1);
			Tcl_Write(errChannel, "\n", 1);
		}
	}

	/* Invoke the script specified on the command line, if any. */

	if (fileName != NULL) {
		code = Tcl_EvalFile(interp, fileName);
		if (code != TCL_OK) {
			errChannel = Tcl_GetStdChannel(TCL_STDERR);
			if (errChannel) {
				/* The following statement guarantees that the errorInfo
				   variable is set properly. */
				Tcl_AddErrorInfo(interp, "");
				Tcl_Write(errChannel,
					  Tcl_GetVar(interp, "errorInfo",
						     TCL_GLOBAL_ONLY), -1);
				Tcl_Write(errChannel, "\n", 1);
			}
			exitCode = 1;
		}
		goto done;
	}

	/* Tcl 7.5 did not support this command */
#if (TCL_MAJOR_VERSION > 7) || (TCL_MINOR_VERSION > 5)

	/* We're running interactively.  Source a user-specific startup file if
	   the application specified one and if the file exists. */

	Tcl_SourceRCFile(interp);
#endif

	/* Loop infinitely, waiting for commands to execute.  When there are no
	   windows left, Tk_MainLoop returns and we exit. */

#ifdef TK
	if (!usetk) {
#endif

		/* Process commands from stdin until there's an end-of-file.  Note
		   that we need to fetch the standard channels again after every
		   eval, since they may have been changed. */

		do_readline();

#ifdef TK
	}
#endif

      done:

#ifdef TK
	if (usetk) {
		Tcl_VarEval(interp,
			    ".f.t insert end {pilot-debug> }; .f.t set mark insert end",
			    0);
		Tk_MainLoop();
		Tcl_DeleteInterp(interp);
		Tcl_Exit(0);
	}
#endif
	return 0;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
