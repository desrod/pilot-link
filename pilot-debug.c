/* pilot-debug.c:  Pilot debugging console, with optional graphics support
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#ifndef LIBDIR
# define LIBDIR "."
#endif

#ifdef TK
int usetk;
# include "tk.h"
#else
# include "tcl.h"
#endif

#ifndef TCL_ACTIVE
# define TCL_ACTIVE TCL_READABLE
#endif

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

int done = 0;

int Interactive = 1;

static Tcl_Interp * interp;        
static struct Pilot_state state;

int debugger = 0; /* If non-zero, then debugger is thought to be active */
int console = 0;  /* If non-zero, then console is thought to be active 
                     Note: if both console and debugger are thought to be
                           active, only the debugger will be usable. If
                           execution is continued, the console be
                           be usable again. */
int stalestate = 1; /* If non-zero, then the current Pilot state (in particular the registers)
                       should be assumed out-of-date */
                       
int port = 0;

/* Misc utility */
void SetLabel(const char * label, const char * value)
{
#ifdef TK
  if (usetk)
    Tcl_VarEval(interp, label, " configure -text \"", value, "\"",NULL);
#endif
}

char * itoa(int val) {
  static char buf[20];
  sprintf(buf,"%d",val);
  return buf;
}

char * htoa(int val) {
  static char buf[9];
  sprintf(buf,"%8.8X",val);
  return buf;
}

char * h4toa(int val) {
  static char buf[5];
  sprintf(buf,"%4.4X",val);
  return buf;
}

int SayInteractive(char * text)
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

int Say(char * text)
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

int Error(char * text)
{
	Tcl_SetResult(interp, text, TCL_STATIC);
	return TCL_ERROR;
}

void SetModeLabel(void)
{
  SetLabel(".state.halted",debugger ? "Debugger" :
                           console ? "Console" :
                           "None");
}


/* Workhorse function to read input from the Pilot. Called both via Tcl event loop on serial input,
   and explicitly by any function has thinks there should be a packet ready. */
   
void Read_Pilot(ClientData clientData, int mask) {
  unsigned char buf[4096];
  int l;
  
  memset(buf,0,4096);
  l = pi_read(port, buf, 4096);
  
  if (l < 6)
    return;

  puts("From Pilot:");
  dumpdata((unsigned char *)buf, l);
  
  if(buf[2] == 0) { /* SysPkt command */
    if(buf[0] == 1) { /* Console */
      if ((!console) || debugger) {
              Say("Console active\n");
	      console = 1;
	      debugger = 0;
	      SetModeLabel();
	      Tcl_VarEval(interp, "checkin 25",NULL);
      }
      if(buf[4] == 0x7f) { /* Message from Pilot */
      	int i;
		
      	for(i=6;i<l;i++)
      	  if(buf[i] == '\r')
      	    buf[i] = '\n';      	
        /* Insert message into both debug and console windows */
#ifdef TK
	if (usetk) {
          Tcl_VarEval(interp,".f.t insert end \"",buf+6,"\"",NULL);
          Tcl_VarEval(interp,".console.t insert end \"",buf+6,"\"",NULL);
          Tcl_VarEval(interp,".f.t mark set insert end",NULL);
          Tcl_VarEval(interp,".console.t mark set insert end",NULL);
          Tcl_VarEval(interp,".f.t see insert",NULL);
          Tcl_VarEval(interp,".console.t see insert",NULL);
        } else
#endif
        Tcl_VarEval(interp,"Say \"",buf+6,"\"",NULL);
      }
    } else if (buf[0] == 0) { /* Debug */
      if (!debugger) {
           debugger = 1;
    	   SetModeLabel();
	   Tcl_VarEval(interp, "checkin 25",NULL);
      }
    	if (buf[4] == 0x7f) { /* Message */
          int i;
          for(i=6;i<l;i++)
      	    if(buf[i] == '\r')
      	      buf[i] = '\n';      	
#ifdef TK
          if (usetk) {
     	    /* Insert message into debug window */
 	    Tcl_VarEval(interp,".f.t insert end \"",buf+6,"\"",NULL);
            Tcl_VarEval(interp,".f.t mark set insert end",NULL);
            Tcl_VarEval(interp,".f.t see insert",NULL);
          } else
#endif
          Tcl_VarEval(interp,"Say \"",buf+6,"\"",NULL);
    	}
    	else if (buf[4] == 0x8c) { /* Breakpoints set */
    	   Say("Breakpoint set\n");
    	}
    	else if (buf[4] == 0x80) { /* State response */
    	
    	   sys_UnpackState(buf+6,&state);
    	   
    	   if (stalestate) {
    	   	char buffer[40];
    	   	sprintf(buffer, "Pilot halted at %8.8lX (function %s) with exception %d\n",
    	   		state.regs.PC, state.func_name, state.exception);
    	   	Say(buffer);
    	   	stalestate = 0;
    	   }
    	   
    	   SetLabel(".state.exception",itoa(state.exception));
    	   SetLabel(".state.funcname",state.func_name);
    	   SetLabel(".state.funcstart",htoa(state.func_start));
    	   SetLabel(".state.funcend",htoa(state.func_end));
    	   SetLabel(".state.d0",htoa(state.regs.D[0]));
    	   SetLabel(".state.a0",htoa(state.regs.A[0]));
    	   SetLabel(".state.d1",htoa(state.regs.D[1]));
    	   SetLabel(".state.a1",htoa(state.regs.A[1]));
    	   SetLabel(".state.d2",htoa(state.regs.D[2]));
    	   SetLabel(".state.a2",htoa(state.regs.A[2]));
    	   SetLabel(".state.d3",htoa(state.regs.D[3]));
    	   SetLabel(".state.a3",htoa(state.regs.A[3]));
    	   SetLabel(".state.d4",htoa(state.regs.D[4]));
    	   SetLabel(".state.a4",htoa(state.regs.A[4]));
    	   SetLabel(".state.d5",htoa(state.regs.D[5]));
    	   SetLabel(".state.a5",htoa(state.regs.A[5]));
    	   SetLabel(".state.d6",htoa(state.regs.D[6]));
    	   SetLabel(".state.a6",htoa(state.regs.A[6]));
    	   SetLabel(".state.d7",htoa(state.regs.D[7]));

    	   SetLabel(".state.ssp",htoa(state.regs.SSP));
    	   SetLabel(".state.usp",htoa(state.regs.USP));
    	   SetLabel(".state.sr",h4toa(state.regs.SR));
    	   SetLabel(".state.pc",htoa(state.regs.PC));

    	   SetLabel(".state.reset",state.reset ? "Yes" : "No");

    	   SetLabel(".state.b1",htoa(state.breakpoint[0].address));
    	   SetLabel(".state.b1a",(state.breakpoint[0].enabled) ? "Yes" : "No");
    	   SetLabel(".state.b2",htoa(state.breakpoint[1].address));
    	   SetLabel(".state.b2a",(state.breakpoint[1].enabled) ? "Yes" : "No");
    	   SetLabel(".state.b3",htoa(state.breakpoint[2].address));
    	   SetLabel(".state.b3a",(state.breakpoint[2].enabled) ? "Yes" : "No");
    	   SetLabel(".state.b4",htoa(state.breakpoint[3].address));
    	   SetLabel(".state.b4a",(state.breakpoint[3].enabled) ? "Yes" : "No");
    	   SetLabel(".state.b5",htoa(state.breakpoint[4].address));
    	   SetLabel(".state.b5a",(state.breakpoint[4].enabled) ? "Yes" : "No");
    	   SetLabel(".state.b6",htoa(state.breakpoint[5].address));
    	   SetLabel(".state.b6a",(state.breakpoint[5].enabled) ? "Yes" : "No");
    	   
    	   /* Show the state window if it is hidden */
    	   Tcl_VarEval(interp, "set show(.state) 1", NULL); 
    	}
    }
  }
}



/* Workhorse function to turn an address string into a numeric value. Ideally, it should
   also know about traps, function names, etc. As it is, it just assumed the text is
   a hexadecimal number, with or with '0x' prefix. */
unsigned long ParseAddress(char * address) {
	return strtoul(address, 0, 16);
}

/* Utility function to modify breakpoint table */
int SetBreakpoint(int bp, unsigned long address, int enabled) {

	state.breakpoint[bp].address = address;
	state.breakpoint[bp].enabled = enabled;

    	SetLabel(".state.b1",htoa(state.breakpoint[0].address));
    	SetLabel(".state.b1a",(state.breakpoint[0].enabled) ? "Yes" : "No");
    	SetLabel(".state.b2",htoa(state.breakpoint[1].address));
    	SetLabel(".state.b2a",(state.breakpoint[1].enabled) ? "Yes" : "No");
    	SetLabel(".state.b3",htoa(state.breakpoint[2].address));
    	SetLabel(".state.b3a",(state.breakpoint[2].enabled) ? "Yes" : "No");
    	SetLabel(".state.b4",htoa(state.breakpoint[3].address));
    	SetLabel(".state.b4a",(state.breakpoint[3].enabled) ? "Yes" : "No");
    	SetLabel(".state.b5",htoa(state.breakpoint[4].address));
    	SetLabel(".state.b5a",(state.breakpoint[4].enabled) ? "Yes" : "No");
    	SetLabel(".state.b6",htoa(state.breakpoint[5].address));
    	SetLabel(".state.b6a",(state.breakpoint[5].enabled) ? "Yes" : "No");
    	
	
	sys_SetBreakpoints(port, &state.breakpoint[0]);
	
	Read_Pilot(0, 0);
	
	return 0;
}

/* Attempt to verify a connection to either the debugger or console */
int DbgAttach(int verify)
{
  struct RPC_params p;
  
  if (!port) {
    Error("No serial port selected, use 'port' command to choose one.\n");
    return 0;
  }

again:  
  if (verify || (!debugger && !console)) {
    int old = debugger;
    sys_QueryState(port);
    debugger = 0;
    Read_Pilot(0,0);
    if (debugger && !old)
      if (verify > 1)
        Say("Attaching to Pilot debugger\n");
      else
        SayInteractive("(attaching to Pilot debugger)\n");
  }
  
  if (!debugger && (verify || !console)) {
    int err;
    int old = console;
     console = 0;
    PackRPC(&p, 0xA09E, RPC_IntReply, RPC_End); /* TaskID, a harmless call */
    DoRPC(port, 1, &p, &err);
    if (err == 0)
      console = 1;
    else 
      console = 0;
    if (console && !old)
      if (verify > 1)
        Say("Attaching to Pilot console\n");
      else
        SayInteractive("(attaching to Pilot console)\n");
    
  }
  
  if (!debugger && !console && !verify) {
    verify = 1;
    goto again;
  }
  
  SetModeLabel();
 
  return (debugger || console);
}

int DbgAttachDebugger(int verify)
{
  if (!port) {
    Error("No serial port selected, use 'port' command to choose one.\n");
    return 0;
  }
  
  if (!debugger || verify) {
    int old = debugger;
    debugger = 0;
    sys_QueryState(port);
    Read_Pilot(0,0);
    SetModeLabel();
    if (debugger && !old)
      SayInteractive("(attaching to Pilot debugger)\n");
  }
  
  if (!debugger) {
    Error("Unable to attach to debugger on Pilot. Is the Pilot connected and in debugging mode?\n");
  }
  return debugger;
}

int DbgAttachConsole(int verify)
{
  int err;
  struct RPC_params p;

  if (!port) {
    Error("No serial port selected, use 'port' command to choose one.\n");
    return 0;
  }
    
  if (verify || debugger || !console) {
    int old = console && !debugger;
    PackRPC(&p, 0xA09E, RPC_IntReply, RPC_End); /* TaskID, a harmless call */
    DoRPC(port, 1, &p, &err);
    if (err == 0) {
       console = 1;
       debugger = 0;
       SetModeLabel();
    } else {
      if (!debugger)
        console = 0;
    }
    if ((console && !debugger) && !old)
      SayInteractive("(attaching to Pilot console)\n");
  }
  
  if (!console || debugger) {
    Error("Unable to attach to console on Pilot. Is the Pilot connected, in console mode, and not in debugger mode?\n");
  }
  
  return console;
}

unsigned long DbgRPC(struct RPC_params * p, int * error)
{
	unsigned long result = 0;
	int err = -1;

  if (!port) {
    Error("No serial port selected, use 'port' command to choose one.\n");
    return 0;
  }
  	
	if (p->reply == RPC_NoReply) {
	  /* If the RPC call will normally generate no reply (as in a call that reboots the machine)
	     then we need to do some special work to verify that the connection is active */
	  DbgAttach(1);
	  if (!debugger && !console) {
	    if (error)
	      *error = -1;
	    Error("Unable to invoke RPC on Pilot. Is the Pilot connected and in debugging or console mode?\n");
	    return 0;
	  }
	}

	if (debugger) {
		result = DoRPC(port, 0, p, &err);
		if ((err < 0) && (p->reply != RPC_NoReply)) /* Failure, assume no response */ {
			debugger = 0;
			SetModeLabel();
		}
	} else if (console) {
		result = DoRPC(port, 1, p, &err);
		if ((err < 0) && (p->reply != RPC_NoReply)) /* Failure, assume no response */ {
			console = 0;
			SetModeLabel();
		}
	}/* else {
		Say("(attaching to Pilot)\n");
	}*/
	
	if (!console && !debugger) {
		DbgAttach(0);
		if (debugger) {
			result = DoRPC(port, 0, p, &err);
		} else if (console) {
			result = DoRPC(port, 1, p, &err);
		} else {
			/* complete failure to attach */
			Error("Unable to invoke RPC on Pilot. Is the Pilot connected and in debugging or console mode?\n");
		}
	}
	
	if (error)
		*error = err;
		
	return result;
}

unsigned char buffer[0xffff];


/* Go, restart execution */
int proc_g(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  /* Use verify since the sys_Continue command produces no return value */
     
  if (!DbgAttachDebugger(1))
    return TCL_ERROR;

  if (argc==2) {
    /* argv[1] is address to start execution at */
    state.regs.PC = ParseAddress(argv[1]);
    SetLabel(".state.pc",htoa(state.regs.PC));
    
    /*SetBreakpoint(port, 5, ParseAddress(argv[1]), 1);*/
  }
  
  sys_Continue(port, &state.regs, 0);
  
  Say("Resuming execution\n");
  
  /* Assume the Pilot is no longer halted */
  
  debugger = 0;
  console = 0;
  stalestate = 1;
  SetModeLabel();

  Tcl_VarEval(interp, "checkupin 25",NULL);
  Tcl_ResetResult(interp);

  return TCL_OK;
}

/* Till, restart execution */
int proc_t(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  /* Use verify since the sys_Continue command produces no return value */
     
  if (!DbgAttachDebugger(1))
    return TCL_ERROR;

  if (argc>=2) {
    /* argv[1] is address to stop execution at */
    SetBreakpoint(5, ParseAddress(argv[1]), 1);
  }
   
  if (argc>=3) {
    /* argv[2] is address to start execution at */
    state.regs.PC = ParseAddress(argv[2]);
    SetLabel(".state.pc",htoa(state.regs.PC));
  }
  
  sys_Continue(port, &state.regs, 0);
  
  Say("Resuming execution\n");
  
  /* Assume the Pilot is no longer halted */
  
  debugger = 0;
  console = 0;
  stalestate = 1;
  SetModeLabel();

  Tcl_VarEval(interp, "checkupin 25",NULL);
  Tcl_ResetResult(interp);

  return TCL_OK;
}

/* Attach to a Pilot that has already crashed into the debugger without notifying us */
int proc_attach(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  DbgAttach(2); /* Two means explicit verify, as opposed to implicit */
  
  if (!debugger && !console) {
     Say("Unable to attach to to Pilot. Is the Pilot connected and in debugging or console mode?\n");
  }

  return TCL_OK;
}

int proc_coldboot(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  struct RPC_params p;

  PackRPC(&p, 0xA08B, RPC_NoReply, RPC_Long(0),RPC_Long(0),RPC_Long(0),RPC_Long(0), RPC_End);
  DbgRPC(&p, 0);

  /* And sever attachment */
  debugger = 0;
  console = 0;
  stalestate = 1;
  SetModeLabel();

  Tcl_VarEval(interp, "checkupin 25",NULL);
  Tcl_ResetResult(interp);

  return TCL_OK;
}

int proc_warmboot(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  struct RPC_params p;

  PackRPC(&p, 0xA08C, RPC_NoReply, RPC_End);
  DbgRPC(&p, 0);
  
  /* And sever attachment */
  debugger = 0;
  console = 0;
  stalestate = 1;
  SetModeLabel();
  
  Tcl_VarEval(interp, "checkupin 25",NULL);
  Tcl_ResetResult(interp);

  return TCL_OK;
}

int proc_battery(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
    		int err;
    		int warn, critical, maxTicks;
    		int v, kind, pluggedin;
    		char buffer[30];
	  struct RPC_params p;

  if (!DbgAttach(0))
    return TCL_ERROR;

	    	warn = 0x1234;
	    	critical = 0x2345;
	    	maxTicks = 0x3456;
	    	kind = 2;
	    	pluggedin = 3;
  
	    PackRPC(&p,0xA0B6, RPC_IntReply,
	    	RPC_Byte(0), RPC_ShortPtr(&warn), RPC_ShortPtr(&critical),
	    	RPC_ShortPtr(&maxTicks), RPC_BytePtr(&kind), RPC_BytePtr(&pluggedin), RPC_End);
	    	
	 

	  v =  DbgRPC(&p, &err);
	  
	 if (err)
	   return TCL_ERROR;
	  
/*		printf("Volts = %f, crit = %f, warn = %f, ticks = %d, kind = %d, pluggedin= %d, err = %d\n",
			(float)v/100, (float)critical/100, (float)warn/100,
			maxTicks, kind, pluggedin, err);*/
	
	sprintf(buffer,"%.2f %s%s", (float)v/100, (kind == 0) ? "Alkaline" :
	                                          (kind == 1) ? "NiCd" :
	                                          (kind == 2) ? "Lithium" :
	                                          "",
	                                          pluggedin ? " Ext" : "" );
	
	Say(buffer);
	
	SetLabel(".state.battery",buffer);
	
	
#ifdef TK
	if (usetk) 
		Tcl_VarEval(interp,".state.battery configure -fg ", (v <= critical) ? "red" :
		                                                    (v <= warn) ? "darkred" :
		                                                    "blue" , NULL);
#endif	

	Say((v <= critical) ? " power critical!\n" : (v <= warn) ? " power low\n" : "\n");
	
	return TCL_OK;
}

#define DB 0xFFFF0000
#define LSSA 0xFA00  

int proc_getdisplay(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
#ifndef TK
  Say("getdisplay is not available due to pilot-debug being compiled without Tk support");
  return TCL_ERROR;

#else  
  char buffer[0xffff];
  char buffer2[0xffff];
  Tk_PhotoImageBlock block;
  Tk_PhotoHandle handle;
  struct RPC_params p;
  unsigned long addr;
  int e1,e2;
  int l;


  if (!usetk) {
    Say("getdisplay is not usable when graphical display is disabled");
    return TCL_ERROR;
  }
  
  if (!DbgAttach(0))
    return TCL_ERROR;
    
  
  if (debugger) {
  	l = sys_ReadMemory(port, DB+LSSA, 4, buffer);
  	addr = get_long(buffer);
  	l = sys_ReadMemory(port, addr, 160*160/8, buffer);
  } else {
 	 PackRPC(&p,0xA026, RPC_IntReply, RPC_LongPtr(&addr), RPC_Long(DB+LSSA), RPC_Long(4), RPC_End);
	  e1=DbgRPC(&p, &e2);
	  for (l=0;l<160*160/8;l+=64) {
		  PackRPC(&p,0xA026, RPC_IntReply, RPC_Ptr(buffer+l, 128), RPC_Long(addr+l), RPC_Long(128), RPC_End);
		  e1=DbgRPC(&p, &e2);
	  }
  }
  
  block.width = 160;
  block.height = 160;
  block.pitch = 160;
  block.pixelSize = 1;
  block.offset[0] = 0;
  block.offset[1] = 0;
  block.offset[2] = 0; 
  
  handle = Tk_FindPhoto("Case");

  for(l=0;l<160*160;l++) {
  	int p = l/8;
  	int b = 1<<(7-(l%8));
  	buffer2[l] = (buffer[p] & b) ? 0 : 0xff;
  }
  block.pixelPtr = buffer2;
  Tk_PhotoPutBlock(handle, &block, 32, 33, 160, 160);
  
  return TCL_OK;  
#endif
}

int proc_transmit(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
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
  strcpy(buffer+6, argv[1]);
  strcat(buffer+6, "\n");
  
  pi_write(port, buffer, 6+strlen(argv[1])+2);
  
  return TCL_OK;  
}

int proc_pushbutton(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  struct RPC_params p;
  unsigned int key=0, scan=0, mod=0;
  
  if (!DbgAttachConsole(0))
    return TCL_ERROR;
  
  sys_RPCerror = 0;
  switch(atoi(argv[1])) {
    case 1: /* Power */
    	key = 0x0208;
    	mod = 0x08;
    	break;

    case 2: /* Datebook */
        key = 0x0204;
        mod = 0x08;
        break;
    case 3: /* Address */
    	key = 0x0205;
    	mod = 0x08;
        break;
    case 6: /* ToDo */
    	key = 0x0206;
    	mod = 0x08;
        break;
    case 7: /* Memo */
    	key = 0x0207;
    	mod = 0x08;
        break;

    case 4: /* Page Up */
    	key = 0x000b;
    	mod = 0;
        break;
    case 5: /* Page Down */
    	key = 0x000c;
    	mod = 0;
        break;
    default:
    	return Error("Button number out of range (1-7)");
  }

  PackRPC(&p, 0xA12D, RPC_IntReply, RPC_Short(key),RPC_Short(scan),RPC_Short(mod), RPC_End);
  
  DbgRPC(&p, 0);
  
  return TCL_OK;
}

/* Tcl procedure to simulate a pen tap -- primarily used by Remote UI bindings */
int proc_pen(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  int x = atoi(argv[1])-32, y = atoi(argv[2])-33, pen = atoi(argv[3]);
  
  /* Transmit Pen event to Pilot */
  
 /*printf("Pen %d, %d, %d\n", x, y, pen);*/
  
  if (!DbgAttachConsole(1))
    return TCL_ERROR;
  
  sys_RemoteEvent(port, pen, x, y, 0, 0,0,0);
  
  
  return TCL_OK;
}

/* Tcl procedure to simulate a key press -- primarily used by Remote UI bindings */
int proc_key(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  struct RPC_params p;
  int key = argv[1][0];
  
  /* Transmit ASCII key to Pilot */

  /*printf("Key %d\n", key);*/
  
  PackRPC(&p, 0xA12D, RPC_IntReply, RPC_Short(key),RPC_Short(0),RPC_Short(0x0), RPC_End);

  if (key != 0)
    DbgRPC(&p, 0);
  
  return TCL_OK;
}

int proc_port(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  static struct pi_sockaddr laddr;
  static Tcl_Channel channel;
  int fd;
  
  if (argc<2) {
  	if(port)
		Tcl_SetResult(interp, laddr.device, TCL_STATIC);
	return TCL_OK;
  }
  
  if (port) {
    Tcl_DeleteChannelHandler(channel, Read_Pilot, (ClientData)port);
    Tcl_UnregisterChannel(interp, channel);
    pi_close(port);
    port = 0;
  }
  
  if (strcmp(argv[1],"close")==0) {
  	return TCL_OK;
  }
  
  port = pi_socket(PI_AF_SLP, PI_SOCK_RAW, PI_PF_SLP);
  
  laddr.sa_family = PI_AF_SLP;
  strcpy(laddr.device,argv[1]);
  
  if (pi_bind(port, &laddr, sizeof(laddr))==-1) {
    Say("Unable to open port '");
    Say(argv[1]);
    Say("': ");
    Say(Tcl_ErrnoMsg(errno));
    Say("\n");
    port = 0;
    return TCL_ERROR;
  }
  
  fd = pi_sdtofd(port);
                        
  channel = Tcl_MakeFileChannel((ClientData)fd, 0, TCL_READABLE);
  Tcl_RegisterChannel(interp, channel); /* And register it to TCL */
  Tcl_CreateChannelHandler(channel, TCL_READABLE, Read_Pilot, (ClientData)port);
  
  Tcl_SetResult(interp, laddr.device, TCL_STATIC);
  return TCL_OK;
}

int proc_inittkdbg(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
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

  Tcl_VarEval(interp,"\

###  /*** Generate remote UI window ***/

set tkdbg 1

toplevel .remote
wm title .remote {Pilot Remote UI}

catch {
	if {[file exists {pix/case.gif}]} {
		set dir {pix}
	} elseif {[file exists {",LIBDIR,"/pix/case.gif}]} {
		set dir {",LIBDIR,"/pix}
	} else {
		error {No pix}
	}
	image create photo Case -format gif -file \"$dir/case.gif\"
	image create photo B1 -format gif -file \"$dir/b1.gif\"
	image create photo B2 -format gif -file \"$dir/b2.gif\"
	image create photo B3 -format gif -file \"$dir/b3.gif\"
	image create photo B4 -format gif -file \"$dir/b4.gif\"
	image create photo B5 -format gif -file \"$dir/b5.gif\"
	image create photo B6 -format gif -file \"$dir/b6.gif\"
	image create photo B7 -format gif -file \"$dir/b7.gif\"
}

canvas .remote.c -width 221 -height 337

.remote.c create rectangle 27 29 196 260 -outline {red} -tag screen -fill {blue}
.remote.c create rectangle 0 280 18 306 -outline {red} -tag button1 -fill {blue}
.remote.c create oval 23 276 52 307 -outline {red} -tag button2 -fill {blue}
.remote.c create oval 63 276 92 307 -outline {red} -tag button3 -fill {blue}
.remote.c create rectangle 97 277 127 294 -outline {red} -tag button4 -fill {blue}
.remote.c create rectangle 97 300 127 317 -outline {red} -tag button5 -fill {blue}
.remote.c create oval 133 276 162 307 -outline {red} -tag button6 -fill {blue}
.remote.c create oval 171 276 200 307 -outline {red} -tag button7 -fill {blue}

.remote.c create rectangle 33 32 187 253 -outline black -tag screen
.remote.c create rectangle 33 200 61 225 -outline black -tag screen
.remote.c create rectangle 33 225 61 253 -outline black -tag screen
.remote.c create rectangle 160 200 187 225 -outline black -tag screen
.remote.c create rectangle 160 225 187 253 -outline black -tag screen
.remote.c create rectangle 62 200 159 253 -outline black -tag screen

.remote.c bind button1 <ButtonPress-1> {.remote.c itemconfigure button1 -fill green; update; pushbutton 1}
.remote.c bind button1 <ButtonRelease-1> {.remote.c itemconfigure button1 -fill blue}

.remote.c bind button2 <ButtonPress-1> {.remote.c itemconfigure button2 -fill green; update; pushbutton 2}
.remote.c bind button2 <ButtonRelease-1> {.remote.c itemconfigure button2 -fill blue}

.remote.c bind button3 <ButtonPress-1> {.remote.c itemconfigure button3 -fill green; update; pushbutton 3}
.remote.c bind button3 <ButtonRelease-1> {.remote.c itemconfigure button3 -fill blue}

.remote.c bind button4 <ButtonPress-1> {.remote.c itemconfigure button4 -fill green; update; pushbutton 4}
.remote.c bind button4 <ButtonRelease-1> {.remote.c itemconfigure button4 -fill blue}

.remote.c bind button5 <ButtonPress-1> {.remote.c itemconfigure button5 -fill green; update; pushbutton 5}
.remote.c bind button5 <ButtonRelease-1> {.remote.c itemconfigure button5 -fill blue}

.remote.c bind button6 <ButtonPress-1> {.remote.c itemconfigure button6 -fill green; update; pushbutton 6}
.remote.c bind button6 <ButtonRelease-1> {.remote.c itemconfigure button6 -fill blue}

.remote.c bind button7 <ButtonPress-1> {.remote.c itemconfigure button7 -fill green; update; pushbutton 7}
.remote.c bind button7 <ButtonRelease-1> {.remote.c itemconfigure button7 -fill blue}

catch {
	.remote.c create image 0 282 -image B1 -anchor nw -tag downbutton1
	.remote.c create image 22 274 -image B2 -anchor nw -tag downbutton2
	.remote.c create image 60 275 -image B3 -anchor nw -tag downbutton3
	.remote.c create image 93 278 -image B4 -anchor nw -tag downbutton4
	.remote.c create image 95 298 -image B5 -anchor nw -tag downbutton5
	.remote.c create image 131 275 -image B6 -anchor nw -tag downbutton6
	.remote.c create image 169 274 -image B7 -anchor nw -tag downbutton7

	.remote.c create image 0 0 -image Case -anchor nw

	.remote.c bind button1 <ButtonPress-1> {.remote.c raise downbutton1; update; pushbutton 1}
	.remote.c bind button1 <ButtonRelease-1> {.remote.c lower downbutton1}

	.remote.c bind button2 <ButtonPress-1> {.remote.c raise downbutton2; update; pushbutton 2}
	.remote.c bind button2 <ButtonRelease-1> {.remote.c lower downbutton2}

	.remote.c bind button3 <ButtonPress-1> {.remote.c raise downbutton3; update; pushbutton 3}
	.remote.c bind button3 <ButtonRelease-1> {.remote.c lower downbutton3}

	.remote.c bind button4 <ButtonPress-1> {.remote.c raise downbutton4; update; pushbutton 4}
	.remote.c bind button4 <ButtonRelease-1> {.remote.c lower downbutton4}

	.remote.c bind button5 <ButtonPress-1> {.remote.c raise downbutton5; update; pushbutton 5}
	.remote.c bind button5 <ButtonRelease-1> {.remote.c lower downbutton5}

	.remote.c bind button6 <ButtonPress-1> {.remote.c raise downbutton6; update; pushbutton 6}
	.remote.c bind button6 <ButtonRelease-1> {.remote.c lower downbutton6}

	.remote.c bind button7 <ButtonPress-1> {.remote.c raise downbutton7; update; pushbutton 7}
	.remote.c bind button7 <ButtonRelease-1> {.remote.c lower downbutton7}
	
	
	.remote.c itemconfigure button1 -outline {} -fill {}
	.remote.c raise button1
	.remote.c itemconfigure button2 -outline {} -fill {}
	.remote.c raise button2
	.remote.c itemconfigure button3 -outline {} -fill {}
	.remote.c raise button3
	.remote.c itemconfigure button4 -outline {} -fill {}
	.remote.c raise button4
	.remote.c itemconfigure button5 -outline {} -fill {}
	.remote.c raise button5
	.remote.c itemconfigure button6 -outline {} -fill {}
	.remote.c raise button6
	.remote.c itemconfigure button7 -outline {} -fill {}
	.remote.c raise button7
	.remote.c itemconfigure screen -outline {} -fill {}
	.remote.c raise screen
}

pack .remote.c -side top

.remote.c bind screen <ButtonPress-1> {pen %x %y 1}
.remote.c bind screen <B1-Motion> {pen %x %y 1}
.remote.c bind screen <ButtonRelease-1> {pen %x %y 0}

bind .remote <KeyPress> {key %A}

#####  /*** Generate remote console window ***/

toplevel .console
wm title .console \"Pilot Remote Console\"
scrollbar .console.y -orient vertical -command {.console.t yview}
text .console.t -yscrollcommand {.console.y set}
pack .console.t -fill both -expand yes -side left
pack .console.y -fill y -side right
focus .console.t

####  /*** Generate pilot state window ***/

toplevel .state
wm title .state {Pilot State}
label .state.l1 -text {Active mode:}
label .state.l1x -text {Battery:}
label .state.l2 -text {Exception:}
label .state.l25 -text {Reset:}
label .state.l3 -text {Function:}
label .state.l4 -text {F-start:}
label .state.l5 -text {F-end:}
label .state.l6 -text {D0:}
label .state.l7 -text {A0:}
label .state.l8 -text {D1:}
label .state.l9 -text {A1:}
label .state.l10 -text {D2:}
label .state.l11 -text {A2:}
label .state.l12 -text {D3:}
label .state.l13 -text {A3:}
label .state.l14 -text {D4:}
label .state.l15 -text {A4:}
label .state.l16 -text {D5:}
label .state.l17 -text {A5:}
label .state.l18 -text {D6:}
label .state.l19 -text {A6:}
label .state.l20 -text {D7:}

label .state.l21 -text {PC:}
label .state.l22 -text {SR:}
label .state.l23 -text {USP:}
label .state.l24 -text {SSP:}

label .state.halted -text {None}
label .state.battery -text {Unknown}
label .state.exception -text {0}
label .state.reset -text {No}
label .state.funcname -text {}
label .state.funcstart -text {00000000}
label .state.funcend -text {00000000}
label .state.d0 -text {00000000}
label .state.a0 -text {00000000}
label .state.d1 -text {00000000}
label .state.a1 -text {00000000}
label .state.d2 -text {00000000}
label .state.a2 -text {00000000}
label .state.d3 -text {00000000}
label .state.a3 -text {00000000}
label .state.d4 -text {00000000}
label .state.a4 -text {00000000}
label .state.d5 -text {00000000}
label .state.a5 -text {00000000}
label .state.d6 -text {00000000}
label .state.a6 -text {00000000}
label .state.d7 -text {00000000}
label .state.pc -text {00000000}
label .state.sr -text {0000}
label .state.usp -text {00000000}
label .state.ssp -text {00000000}

grid .state.l1 -column 0 -row 0 -sticky e -columnspan 2
grid .state.halted -column 2 -row 0 -sticky w -columnspan 2

grid .state.l1x -column 0 -row 1 -sticky e -columnspan 2
grid .state.battery -column 2 -row 1 -sticky w -columnspan 2

grid .state.l2 -column 0 -row 2 -sticky e -columnspan 2
grid .state.exception -column 2 -row 2 -sticky w -columnspan 2

grid .state.l25 -column 0 -row 3 -sticky e -columnspan 2
grid .state.reset -column 2 -row 3 -sticky w -columnspan 2

grid .state.l3 -column 0 -row 4 -sticky e -columnspan 2 
grid .state.funcname -column 2 -row 4 -sticky w -columnspan 2

grid .state.l4 -column 0 -row 5 -sticky e -columnspan 2 
grid .state.funcstart -column 2 -row 5 -sticky w -columnspan 2

grid .state.l5 -column 0 -row 6 -sticky e -columnspan 2 
grid .state.funcend -column 2 -row 6 -sticky w -columnspan 2

frame .state.rule1 -relief raised -bd 2 -height 4
grid .state.rule1 -column 0 -row 7 -columnspan 4 -sticky ew

grid .state.l6 -column 0 -row 8 -sticky e 
grid .state.d0 -column 1 -row 8 -sticky w
grid .state.l7 -column 2 -row 8 -sticky e 
grid .state.a0 -column 3 -row 8 -sticky w

grid .state.l8 -column 0 -row 9 -sticky e 
grid .state.d1 -column 1 -row 9 -sticky w
grid .state.l9 -column 2 -row 9 -sticky e 
grid .state.a1 -column 3 -row 9 -sticky w

grid .state.l10 -column 0 -row 10 -sticky e 
grid .state.d2 -column 1 -row 10 -sticky w
grid .state.l11 -column 2 -row 10 -sticky e 
grid .state.a2 -column 3 -row 10 -sticky w

grid .state.l12 -column 0 -row 11 -sticky e 
grid .state.d3 -column 1 -row 11 -sticky w
grid .state.l13 -column 2 -row 11 -sticky e 
grid .state.a3 -column 3 -row 11 -sticky w

grid .state.l14 -column 0 -row 12 -sticky e 
grid .state.d4 -column 1 -row 12 -sticky w
grid .state.l15 -column 2 -row 12 -sticky e 
grid .state.a4 -column 3 -row 12 -sticky w

grid .state.l16 -column 0 -row 13 -sticky e 
grid .state.d5 -column 1 -row 13 -sticky w
grid .state.l17 -column 2 -row 13 -sticky e 
grid .state.a5 -column 3 -row 13 -sticky w

grid .state.l18 -column 0 -row 14 -sticky e 
grid .state.d6 -column 1 -row 14 -sticky w
grid .state.l19 -column 2 -row 14 -sticky e 
grid .state.a6 -column 3 -row 14 -sticky w

grid .state.l20 -column 0 -row 15 -sticky e 
grid .state.d7 -column 1 -row 15 -sticky w

grid .state.l21 -column 0 -row 16 -sticky e 
grid .state.pc -column 1 -row 16 -sticky w
grid .state.l22 -column 2 -row 16 -sticky e 
grid .state.sr -column 3 -row 16 -sticky w

grid .state.l23 -column 0 -row 17 -sticky e 
grid .state.usp -column 1 -row 17 -sticky w
grid .state.l24 -column 2 -row 17 -sticky e 
grid .state.ssp -column 3 -row 17 -sticky w

label .state.bl2 -text {B1:}
label .state.bl4 -text {B2:}
label .state.bl6 -text {B3:}
label .state.bl8 -text {B4:}
label .state.bl10 -text {B5:}
label .state.bl12 -text {B6:}

label .state.b1 -text {00000000}
label .state.b1a -text {Off}
label .state.b2 -text {00000000}
label .state.b2a -text {Off}
label .state.b3 -text {00000000}
label .state.b3a -text {Off}
label .state.b4 -text {00000000}
label .state.b4a  -text {Off}
label .state.b5 -text {00000000}
label .state.b5a -text {Off}
label .state.b6 -text {00000000}
label .state.b6a -text {Off}

frame .state.rule2 -relief raised -bd 2 -height 4
grid .state.rule2 -column 0 -row 18 -columnspan 4 -sticky ew

grid .state.bl2 -column 0 -row 19 -sticky e
grid .state.b1 -column 1 -row 19 -sticky w
grid .state.b1a -column 2 -row 19 -sticky w 

grid .state.bl4 -column 0 -row 20 -sticky e
grid .state.b2 -column 1 -row 20 -sticky w
grid .state.b2a -column 2 -row 20 -sticky w 

grid .state.bl6 -column 0 -row 21 -sticky e
grid .state.b3 -column 1 -row 21 -sticky w
grid .state.b3a -column 2 -row 21 -sticky w 

grid .state.bl8 -column 0 -row 22 -sticky e
grid .state.b4 -column 1 -row 22 -sticky w
grid .state.b4a -column 2 -row 22 -sticky w 

grid .state.bl10 -column 0 -row 23 -sticky e
grid .state.b5 -column 1 -row 23 -sticky w
grid .state.b5a -column 2 -row 23 -sticky w 

grid .state.bl12 -column 0 -row 24 -sticky e
grid .state.b6 -column 1 -row 24 -sticky w
grid .state.b6a -column 2 -row 24 -sticky w 

###  /*** Generate debugger console window ***/

wm title . \"Pilot Debugger Console\"
catch {
	wm iconbitmap . {@pix/case.xbm}
	wm iconbitmap .remote {@pix/case.xbm}
	wm iconbitmap .console {@pix/case.xbm}
	wm iconbitmap .state {@pix/case.xbm}
	#wm iconmask . {@pix/casemask.xbm}
}

frame .m -relief raised
frame .f
scrollbar .f.y -orient vertical -command {.f.t yview}
text .f.t -yscrollcommand {.f.y set} -wrap word
pack .f.t -fill both -expand yes -side left
pack .f.y -fill y -side right
pack [menubutton .m.file -text {File} -menu .m.file.m] -side left
#pack [menubutton .m.edit -text {Edit} -menu .m.edit.m] -side left
pack [menubutton .m.windows -text {Windows} -menu .m.windows.m] -side left
#pack [menubutton .m.help -text {Help} -menu .m.help.m] -side right

menu .m.file.m
menu .m.windows.m
#menu .m.edit.m
#menu .m.help.m

#wm iconify .remote
#wm iconify .console
#wm iconify .state


proc ShowWindow {name1 name2 op} {
  global show
  if {$show($name2)} {
    if {[wm state $name2] != \"normal\"} {
      wm deiconify $name2
    }
  } else {
    if {[wm state $name2] == \"normal\"} {
      wm withdraw $name2
    }
  }
}


trace variable show w {ShowWindow}

bind .remote <Unmap> { set show(.remote) 0 }
bind .console <Unmap> { set show(.console) 0 }
bind .state <Unmap> { set show(.state) 0 }

bind .remote <Map> { set show(.remote) 1 }
bind .console <Map> { set show(.console) 1 }
bind .state <Map> { set show(.state) 1 }

wm protocol .remote WM_DELETE_WINDOW { set show(.remote) 0 }
wm protocol .console WM_DELETE_WINDOW { set show(.console) 0 }
wm protocol .state WM_DELETE_WINDOW { set show(.state) 0 }

set show(.remote) 0
set show(.console) 0
set show(.state) 0


.m.file.m add command -label {Exit} -command {exit}

.m.windows.m add checkbutton -label {Remote UI} -var show(.remote)
.m.windows.m add checkbutton -label {Remote Console} -var show(.console)
.m.windows.m add checkbutton -label {Remote State} -var show(.state)

pack .m -side top -fill x
pack .f -side top -fill both -expand yes
focus .f.t

###  /*** Configure console bindings ***/

bind .console.t <Shift-KeyPress-Return> {tkTextInsert .console.t \"\\n\" ; break}
bind .f.t <Shift-KeyPress-Return> {tkTextInsert .f.t \"\\n\" ; break}

bind .console.t <Control-KeyPress-Return> {tkTextInsert .console.t \"\\n\" ; break}
bind .f.t <Control-KeyPress-Return> {tkTextInsert .f.t \"\\n\" ; break}

bind .console.t <KeyPress-Return> {Console [.console.t get {insert linestart} {insert lineend}] ; break}
bind .f.t <KeyPress-Return> {Debugger [.f.t get {insert linestart} {insert lineend}] ; break}

proc Console {cmd} {
	.console.t mark set insert {insert lineend}
	tkTextInsert .console.t \\n
	transmit $cmd
}

proc Debugger {cmd} {
	.f.t mark set insert {insert lineend}
	tkTextInsert .f.t \\n
	if {[string length [string trim $cmd]]!=0} {
		set code [catch {eval $cmd} message]
		if {[string length $message]} {
			set message [string trimright $message]
			Say \"$message\\n\"
			if {[.f.t compare insert != {insert linestart}]} {
				Say \"\\\n\"
			}
		}
	}
}

proc Say {text} {
	global Interactive
	if {$Interactive} {
	  .f.t insert insert \"$text\"
	  .f.t see insert
	} else {
	  upvar result result
	  set result \"$result$text\"
	}
}


", NULL);
  puts(interp->result);
  
  created = 1;
  usetk = 1;

  return TCL_OK;
#endif
}

int proc_help(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
  Say("\
--- Help ---\n\
g [<addr>]\tGo: Resume execution (if address is supplied, will start at that point)\n\
t <addr1> [<addr2>}\tTill: Resume execution until addr1 (if addr2 is supplied, will start at that point)\n\
coldboot\n\
warmboot\n\
pushbutton <button number>\tSimulate button push\n\
getdisplay\tShow the Pilot's display in the Remote UI window\n\
");
  return TCL_OK;
}

struct { char * name; Tcl_CmdProc * proc; } cmds[] = {
	{ "coldboot",	proc_coldboot },
	{ "warmboot",	proc_warmboot },
	{ "pushbutton",	proc_pushbutton },
	{ "pen",	proc_pen },
	{ "key",	proc_key },
	{ "g",		proc_g },
	{ "t",		proc_t },
	{ "attach",	proc_attach },
	{ "transmit",	proc_transmit },
	{ "getdisplay",	proc_getdisplay },
	{ "battery",	proc_battery },
	{ "port",	proc_port },
	{ "help",	proc_help },
	{ "inittkdbg",  proc_inittkdbg },
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
proc Say {text} {
	global Interactive
	if {$Interactive} {
	  puts \"$text\"
	} else {
	  upvar result result
	  set result \"$result$text\"
	}
}

proc interactive {args} {
	global Interactive
	global errorInfo
	set hold $Interactive
	set Interactive 1
	set code [catch $args message]
	set Interactive $hold
	error $message $errorInfo $code
}

proc noninteractive {args} {
	global Interactive
	global errorInfo
	set hold $Interactive
	set Interactive 0
	set code [catch $args message]
	set Interactive $hold
	error $message $errorInfo $code
}

set Interactive 1


proc bgerror {msg} {
	Say $msg
}

proc checkup {} {
	catch {noninteractive battery}
	after 10000 checkup
}

after 1000 checkup

proc checkupin {time} {
	after $time {catch {noninteractive battery}}
}

",NULL);     

  Tcl_LinkVar(interp, "Interactive", (char*)&Interactive, TCL_LINK_INT);


#ifdef TK  
  if (usetk)
    Tcl_VarEval(interp,"inittkdbg",NULL);
  else
    Tcl_VarEval(interp,"set tkdbg 0", NULL);
#endif

  Say("\tWelcome to pilot-debug!\n\nType 'help' for further information.\n\n");
  
#if 0
  Say("\tWelcome to pilot-debug!\n\n\
Please connect your Pilot and start console or debugging mode.\n\n(Console mode is a background \
task that can respond to a few commands, most importantly RPC which lets any function on the \
Pilot be invoked. The Pilot operates as usual while console mode is active, except that \
since the serial port is help open, HotSync and other applications that use the serial port \
will not work. Debug mode is activated on demand or when the Pilot crashes. In debug mode, the \
CPU is halted, and no commands may be executed, except via a debugging console like this one.)\n\n\
In the absence of special utilities, the console can be started by \
the \".2\" shortcut, and debugging via \".1\". To clear either mode, \
reboot via the reset button. If console mode is active, you may \
also reboot via the \"coldboot\" or \"warmboot\" commands.\n\n\
The Remote UI window lets you manipulate the Pilot if console mode is active. By clicking the \
mouse button on the screen or buttons, you can simulate pen taps, and if you type anything \
while the window has the focus, the Pilot will receive the keystrokes.\n\n \
The Remote Console window is specifically for the transmission and reception of console \
packets. Pressing Return on a line will transmit it, and any incoming packets will be \
displayed here in addition to the Debug Console.\n\n\
The Remote State window shows the current Pilot CPU state. It is only updated on request or \
when the Pilot halts.\n\n\
The Debugging Console window is the primary interface for pilot-debug. Pressing Return on a \
line that contains text will execute that line as a Tcl command. (Try 'expr 3+4'.) All of \
the usual Tcl and Tk commands are available, as well as some special-purpose ones, including \
'help', 'coldboot', 'warmboot', 'attach', 't', and 'g', (the last one continues after the Pilot halts.)\n\n\
Execute 'help' for the list of commands currently implemented.\n\
");
#endif

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    Tcl_SetVar(interp, "tcl_rcFileName", "~/.pdebugrc", TCL_GLOBAL_ONLY);

    Tcl_VarEval(interp,"\
      set tcl_prompt1 myprompt
      proc myprompt {} {
        puts -nonewline \"pilot-debug> \"
      }
      
    ",NULL);

    /* Deal with command-line arguments */

    Tcl_VarEval(interp,"\
      if {$argc > 0} {
        set p [lindex $argv 0]
        set argv [lrange $argv 1 end]
        port $p
      } else {
        Say \"As you have not entered a serial port on the command like, you might like to \
set one with 'port /dev/something'\\n\\n\"
      }
    ",NULL);
        
    return TCL_OK;
}

static void
StdinProc(ClientData clientData, int mask);

static void
Prompt(Tcl_Interp * interp, int partial);


static Tcl_DString command;	/* Used to buffer incomplete commands being
				 * read from stdin. */
static Tcl_DString line;	/* Used to read the next line from the
                                 * terminal input. */
static int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's*/


int main(int argc, char *argv[])
{
    char *args, *fileName;
    char buf[20];
    int code;
    size_t length;
    int exitCode = 0;
    Tcl_Channel inChannel, outChannel, errChannel;

    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();

    fileName = NULL;
    if (argc > 2) {
	length = strlen(argv[1]);
	if ((length >= 2) && (strncmp(argv[1], "-file", length) == 0)) {
	    fileName = argv[2];
	    argc-=2;
	    argv+=2;
	}
    }

#ifdef TK    
    usetk = 1;
#endif
    
    if (argc > 1) {
        length = strlen(argv[1]);
        if ((length >= 2) && (strncmp(argv[1], "-notk", length) == 0)) {
#ifdef TK
             usetk = 0;
#endif
             argc--;
             argv++;
        }
    }

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".
     */

    args = Tcl_Merge(argc-1, argv+1);
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    sprintf(buf, "%d", argc-1);
    Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv[0],
	    TCL_GLOBAL_ONLY);

    /*
     * Set the "tcl_interactive" variable.
     */

    tty = isatty(0);
    Tcl_SetVar(interp, "tcl_interactive",
	    ((fileName == NULL) && tty) ? "1" : "0", TCL_GLOBAL_ONLY);


#ifdef TK
    if (!getenv("DISPLAY") || (strlen(getenv("DISPLAY"))==0))
        usetk = 0;
#endif

    /*
     * Invoke application-specific initialization.
     */

    if (Tcl_AppInit(interp) != TCL_OK) {
	errChannel = Tcl_GetStdChannel(TCL_STDERR);
	if (errChannel) {
	    Tcl_Write(errChannel,
		    "application-specific initialization failed: ", -1);
	    Tcl_Write(errChannel, interp->result, -1);
	    Tcl_Write(errChannel, "\n", 1);
	}
    }

    /*
     * Invoke the script specified on the command line, if any.
     */

    if (fileName != NULL) {
	code = Tcl_EvalFile(interp, fileName);
	if (code != TCL_OK) {
	    errChannel = Tcl_GetStdChannel(TCL_STDERR);
	    if (errChannel) {
		/*
		 * The following statement guarantees that the errorInfo
		 * variable is set properly.
		 */

		Tcl_AddErrorInfo(interp, "");
		Tcl_Write(errChannel,
			Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY), -1);
		Tcl_Write(errChannel, "\n", 1);
	    }
	    exitCode = 1;
	}
	goto done;
    }

    /*
     * We're running interactively.  Source a user-specific startup
     * file if the application specified one and if the file exists.
     */

    Tcl_SourceRCFile(interp);
    
    /*
     * Loop infinitely, waiting for commands to execute.  When there
     * are no windows left, Tk_MainLoop returns and we exit.
     */
     
#ifdef TK
    if (!usetk) {     
#endif

    /*
     * Process commands from stdin until there's an end-of-file.  Note
     * that we need to fetch the standard channels again after every
     * eval, since they may have been changed.
     */

    inChannel = Tcl_GetChannel(interp, "stdin", NULL);
    if (inChannel) {
	Tcl_CreateChannelHandler(inChannel, TCL_READABLE|TCL_ACTIVE,
		StdinProc, (ClientData) inChannel);
    }
    if (tty) {
	Prompt(interp, 0);
    }
    outChannel = Tcl_GetChannel(interp, "stdout", NULL);
    if (outChannel) {
	Tcl_Flush(outChannel);
    }
    Tcl_DStringInit(&command);
    Tcl_CreateExitHandler((Tcl_ExitProc *) Tcl_DStringFree, (ClientData) &command);
    Tcl_DStringInit(&line);
    Tcl_ResetResult(interp);
 
    /*
     * Loop infinitely until all event handlers are passive. Then exit.
     * Rather than calling exit, invoke the "exit" command so that
     * users can replace "exit" with some other command to do additional
     * cleanup on exit.  The Tcl_Eval call should never return.
     */
     
#ifdef TK
    }
#endif

done:

#ifdef TK
    if (usetk) {
        Tk_MainLoop();
        Tcl_DeleteInterp(interp);
        Tcl_Exit(0);
    } else {
#endif
        while (Tcl_DoOneEvent(0)) {
        }
        sprintf(buffer, "exit %d", exitCode);
        Tcl_Eval(interp, buffer);
#ifdef TK
    }
#endif
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(clientData, mask)
    ClientData clientData;		/* Not used. */
    int mask;				/* Not used. */
{
    static int gotPartial = 0;
    char *cmd;
    int code, count;
    Tcl_Channel newchan, chan = (Tcl_Channel) clientData;

    count = Tcl_Gets(chan, &line);

    if (count < 0) {
	if (!gotPartial) {
	    if (tty) {
		Tcl_Exit(0);
	    } else {
		Tcl_DeleteChannelHandler(chan, StdinProc, (ClientData) chan);
	    }
	    return;
	} else {
	    count = 0;
	}
    }

    (void) Tcl_DStringAppend(&command, Tcl_DStringValue(&line), -1);
    cmd = Tcl_DStringAppend(&command, "\n", -1);
    Tcl_DStringFree(&line);
    
    if (!Tcl_CommandComplete(cmd)) {
        gotPartial = 1;
        goto prompt;
    }
    gotPartial = 0;

    /*
     * Disable the stdin channel handler while evaluating the command;
     * otherwise if the command re-enters the event loop we might
     * process commands from stdin before the current command is
     * finished.  Among other things, this will trash the text of the
     * command being evaluated.
     */

    Tcl_CreateChannelHandler(chan, TCL_ACTIVE, StdinProc, (ClientData) chan);
    code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
    newchan = Tcl_GetChannel(interp, "stdin", NULL);
    if (chan != newchan) {
	Tcl_DeleteChannelHandler(chan, StdinProc, (ClientData) chan);
    }
    if (newchan) {
	Tcl_CreateChannelHandler(newchan, TCL_READABLE | TCL_ACTIVE,
		StdinProc, (ClientData) newchan);
    }
    Tcl_DStringFree(&command);
    if (*interp->result != 0) {
	if (code != TCL_OK) {
	    chan = Tcl_GetChannel(interp, "stderr", NULL);
	} else if (tty) {
	    chan = Tcl_GetChannel(interp, "stdout", NULL);
	} else {
	    chan = NULL;
	}
	if (chan) {
	    Tcl_Write(chan, interp->result, -1);
	    Tcl_Write(chan, "\n", 1);
	}
    }

    /*
     * Output a prompt.
     */

    prompt:
    if (tty) {
	Prompt(interp, gotPartial);
    }
    Tcl_ResetResult(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Prompt --
 *
 *	Issue a prompt on standard output, or invoke a script
 *	to issue the prompt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A prompt gets output, and a Tcl script may be evaluated
 *	in interp.
 *
 *----------------------------------------------------------------------
 */

static void
Prompt(interp, partial)
    Tcl_Interp *interp;			/* Interpreter to use for prompting. */
    int partial;			/* Non-zero means there already
					 * exists a partial command, so use
					 * the secondary prompt. */
{
    char *promptCmd;
    int code;
    Tcl_Channel outChannel, errChannel;

    errChannel = Tcl_GetChannel(interp, "stderr", NULL);

    promptCmd = Tcl_GetVar(interp,
	partial ? "tcl_prompt2" : "tcl_prompt1", TCL_GLOBAL_ONLY);
    if (promptCmd == NULL) {
	outChannel = Tcl_GetChannel(interp, "stdout", NULL);
defaultPrompt:
	if (!partial && outChannel) {
            Tcl_Write(outChannel, "% ", 2);
	}
    } else {
	code = Tcl_Eval(interp, promptCmd);
	outChannel = Tcl_GetChannel(interp, "stdout", NULL);
	if (code != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (script that generates prompt)");
            /*
             * We must check that errChannel is a real channel - it
             * is possible that someone has transferred stderr out of
             * this interpreter with "interp transfer".
             */

	    errChannel = Tcl_GetChannel(interp, "stdout", NULL);
            if (errChannel != (Tcl_Channel) NULL) {
                Tcl_Write(errChannel, interp->result, -1);
                Tcl_Write(errChannel, "\n", 1);
            }
	    goto defaultPrompt;
	} else if (*interp->result && outChannel) {
	    Tcl_Write(outChannel, interp->result, strlen(interp->result));
	}
    }
    if (outChannel) {
        Tcl_Flush(outChannel);
    }
}
