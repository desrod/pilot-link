/* pd-tty.c: Text asynchronous input/output support for pilot-debug. 
 *           Currently includes interfaces to STDIO (using plus-patch style
 *           handlers), readline 2.0 (using internal hack to simulate
 *           co-routine), and Tk (using a standard text widget.)
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 * 
 */

void display(char * text, char * tag, int type);
void do_readline(void);
 
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

#ifdef TK
extern int usetk;
# include "tk.h"
#else
# include "tcl.h"
#endif

#ifndef TCL_ACTIVE
# define TCL_ACTIVE TCL_READABLE
#endif

extern int Interactive;

extern Tcl_Interp * interp;        

extern int tty;			/* Non-zero means standard input is a
				 * terminal-like device.  Zero means it's*/

#ifdef READLINE_2_0

#include <readline/readline.h>
#include <readline/history.h>

/* Undocumented readline-2.0 internals */
extern void rl_deprep_terminal(void);
extern int rl_getc(FILE * stream);
extern void rl_gather_tyi(void);

static volatile int readable = 0;

static Tcl_DString command;

void Readable(ClientData d, int mask) { readable = 1; }

void Exit(ClientData d) { rl_deprep_terminal(); }

static int mode = 0;

void do_readline(void)
{
   char buf[20];
   int gotPartial = 0;
   int exitCode = 0;
   Tcl_Channel in = Tcl_GetStdChannel(TCL_STDIN);
   
   Tcl_SetChannelOption(interp, in, "-blocking", "off");
   
   Tcl_CreateChannelHandler(in, TCL_READABLE, Readable, 0);
   
   for(;;) {
      char * line = readline(gotPartial ? "> " : "pilot-debug> ");
      char * cmd;
      int code;
      if (!line)
         break;
      (void) Tcl_DStringAppend(&command, line, -1);
      cmd = Tcl_DStringAppend(&command, "\n", -1);
      
      add_history(line);
      free(line);
      
      if (!Tcl_CommandComplete(cmd)) {
         gotPartial = 1;
      } else {
         gotPartial = 0;
         code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
         
         
         Tcl_DStringFree(&command);
         if (*interp->result != 0) {
            Tcl_Channel chan;
            if (code != TCL_OK) {
               chan = Tcl_GetChannel(interp, "stderr", NULL);
            } else {
               chan = Tcl_GetChannel(interp, "stdout", NULL);
            }
            if (chan) {
               Tcl_Write(chan, interp->result, -1);
               Tcl_Write(chan, "\n", 1);
            }
         }
      }
      Tcl_ResetResult(interp);
   }
   sprintf(buf, "exit %d", exitCode);
   Tcl_Eval(interp, buf);
}


/* Replace internal readline routine that retrieves a character */
int rl_getc(FILE * stream)
{
   unsigned char c;
   int d;
   
   for(;;) {
      if (!readable)
      	Tcl_DoOneEvent(0);
      d = Tcl_Read(Tcl_GetStdChannel(TCL_STDIN), &c, 1);
      readable = 0;
      if (d == 1) {
         if (mode) {
            printf("\n");
            mode = 0;
            rl_forced_update_display();
         }
         return (unsigned int)c;
      }
   }
}

/* Replace internal readline routine that gets a character without blocking */
void rl_gather_tyi(void)
{
   unsigned char c;
   int d;
   
   Tcl_DoOneEvent(TCL_DONT_WAIT);
   d = Tcl_Read(Tcl_GetStdChannel(TCL_STDIN), &c, 1);
   if (d == 1) {
      if (mode) {
         printf("\n");
         mode = 0;
         rl_forced_update_display();
      }
      rl_stuff_char(c);
   }
   return;
}
#else

static void
StdinProc(ClientData clientData, int mask);

static void
Prompt(Tcl_Interp * interp, int partial);


static Tcl_DString command;	/* Used to buffer incomplete commands being
				 * read from stdin. */
static Tcl_DString line;	/* Used to read the next line from the
                                 * terminal input. */

static int gotPartial = 0;

static int mode = 0;


void do_readline(void)
{
    char buf[20];
    int exitCode = 0;
    Tcl_Channel inChannel, outChannel;

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
     
    while (Tcl_DoOneEvent(0)) {
    }
    sprintf(buf, "exit %d", exitCode);
    Tcl_Eval(interp, buf);
    return;
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



#endif /*!USE_READLINE_2_0*/

void display(char * text, char * tag, int type)
{
   int i;

#ifdef TK
   if (usetk) {
      Tcl_DString disp;
      Tcl_DStringInit(&disp);
      if (mode == 0) {      	
         Tcl_DStringAppend(&disp,".f.t mark set display {insert linestart};", -1);
         mode = 1;
      }
      Tcl_DStringAppend(&disp, ".f.t insert display",-1);
      Tcl_DStringAppendElement(&disp, text);
      if (tag)
         Tcl_DStringAppendElement(&disp, tag);
      if (strlen(text) && (text[strlen(text)-1] == '\n')) {
         mode = 0;
      }
      if (mode == 0)
         Tcl_DStringAppend(&disp,";.f.t mark unset display", -1);
      /*printf("Exec |%s|\n", Tcl_DStringValue(&disp));*/
      Tcl_Eval(interp,Tcl_DStringValue(&disp));
      /*puts(interp->result);*/
      Tcl_DStringFree(&disp);
      return;
   }
#endif   
   type++;
   
   for (i=0;i<strlen(text);i++) {
      if (mode == 0) {
         /* At prompt */
         /* Dumb hack to erase prompt */
         printf("\r                                               \r");
         mode = -1; /* Beginning of line */
      }
      if (mode != type) {
         if (mode != -1)
            printf("\n"); /* end current line */
         printf("%s", tag);
         mode = type;
      }
      printf("%c", text[i]);
      if (text[i] == '\n') {
        mode = 0; 
#ifdef READLINE_2_0
        rl_forced_update_display(); /* Bring the prompt back */
#else
	Prompt(interp, gotPartial);
#endif        
      }
   }
}


#if 0
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
#endif
