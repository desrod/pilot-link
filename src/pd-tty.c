/* 
 * pd-tty.c: Text asynchronous input/output support for pilot-debug. 
 *           Currently includes interfaces to STDIO (using plus-patch style
 *           handlers), readline 2.0 (using hack of readline internals to
 *           simulate co-routine), readline 2.1 (using callback mechanism)
 *           and Tk (using a standard text widget.)
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

void display(char *text, char *tag, int type);
void do_readline(void);

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "pi-source.h"
#include "pi-dlp.h"
#include "pi-syspkt.h"

#ifdef TK
extern int usetk;

#include <tk.h>
#else
#include <tcl.h>
#endif

#ifndef TCL_ACTIVE
#define TCL_ACTIVE TCL_READABLE
#endif

extern int Interactive;

extern Tcl_Interp *interp;

extern int tty;			/* Non-zero means standard input is a
				   terminal-like device.  Zero means it's 
				   NULL */

#ifdef HAVE_READLINE_EXTRA

#include <readline/readline.h>
#include <readline/history.h>

static Tcl_DString command;

static int mode = 0;

static void Readable(ClientData d, int mask)
{
	mode = 1;
	rl_callback_read_char();
}

static void Exit(ClientData d)
{
	rl_callback_handler_remove();
	if (mode)
		puts("");
}

/***********************************************************************
 *
 * Function:    execline
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void execline(char *line)
{
	int 	gotPartial 	= 0;
	char 	*cmd;

	if (line == 0) {
		char buf[20];
		int 	exitCode = 0;

		sprintf(buf, "exit %d", exitCode);
		Tcl_Eval(interp, buf);
		return;
	}
	(void) Tcl_DStringAppend(&command, line, -1);
	cmd = Tcl_DStringAppend(&command, "\n", -1);

	add_history(line);

	if (!Tcl_CommandComplete(cmd)) {
		gotPartial = 1;
	} else {
		int 	code;

		gotPartial 	= 0;
		mode 		= 0;
		code 		= Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);

		Tcl_DStringFree(&command);
		if (*interp->result != 0) {
			Tcl_Channel chan;

			if (code != TCL_OK) {
				chan =
				    Tcl_GetChannel(interp, "stderr", NULL);
			} else {
				chan =
				    Tcl_GetChannel(interp, "stdout", NULL);
			}
			if (chan) {
				Tcl_Write(chan, interp->result, -1);
				Tcl_Write(chan, "\n", 1);
			}
		}
		Tcl_ResetResult(interp);
	}

	rl_callback_handler_install(gotPartial ? "> " : "pilot-debug> ",
				    execline);
}

/***********************************************************************
 *
 * Function:    do_readline
 *
 * Summary:     Internal readline routine
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void do_readline(void)
{
	char 	buf[20];
	int 	exitCode 	= 0;
	
	Tcl_Channel in = Tcl_GetStdChannel(TCL_STDIN);

	Tcl_SetChannelOption(interp, in, "-blocking", "off");

	Tcl_CreateChannelHandler(in, TCL_READABLE, Readable, 0);

	rl_callback_handler_install("pilot-debug> ", execline);

	Tcl_CreateExitHandler((Tcl_ExitProc *) Exit, (ClientData) 0);

	while (Tcl_DoOneEvent(0)) {
	}

	sprintf(buf, "exit %d", exitCode);
	Tcl_Eval(interp, buf);
	return;

}

#else

static void StdinProc(ClientData clientData, int mask);
static void Prompt(Tcl_Interp * interp, int partial);

static Tcl_DString command;	/* Used to buffer incomplete commands being
				   read from stdin. */

static Tcl_DString line;	/* Used to read the next line from the
				   terminal input. */

static int gotPartial 	= 0;
static int mode 	= 0;

/***********************************************************************
 *
 * Function:    do_readline
 *
 * Summary:     Process commands from stdin until there's an end-of-file
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void do_readline(void)
{
	char 	buf[20];
	int 	exitCode 	= 0;
	
	Tcl_Channel inChannel, outChannel;

	/* Note that we need to fetch the standard channels again after 
	   every eval, since they may have been changed. */

	inChannel = Tcl_GetChannel(interp, "stdin", NULL);
	if (inChannel) {
		Tcl_CreateChannelHandler(inChannel,
					 TCL_READABLE | TCL_ACTIVE,
					 StdinProc,
					 (ClientData) inChannel);
	}
	if (tty) {
		Prompt(interp, 0);
	}
	outChannel = Tcl_GetChannel(interp, "stdout", NULL);
	if (outChannel) {
		Tcl_Flush(outChannel);
	}
	Tcl_DStringInit(&command);
	Tcl_CreateExitHandler((Tcl_ExitProc *) Tcl_DStringFree,
			      (ClientData) & command);
	Tcl_DStringInit(&line);
	Tcl_ResetResult(interp);

	/* Loop infinitely until all event handlers are passive. Then exit. 
	   Rather than calling exit, invoke the "exit" command so that users
	   can replace "exit" with some other command to do additional
	   cleanup on exit.  The Tcl_Eval call should never return. */

	while (Tcl_DoOneEvent(0)) {
	}
	sprintf(buf, "exit %d", exitCode);
	Tcl_Eval(interp, buf);
	return;
}



/***********************************************************************
 *
 * Function:    StdinProc
 *
 * Summary:     This procedure is invoked by the event dispatcher whenever
 *              standard input becomes readable.  It grabs the next line of
 *              input chcters, adds them to a command being assembled,
 *              and executes the command if it's complete.
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 * Comments:    Could be almost arbitrary, depending on the command 
 *		that's used
 *
 ***********************************************************************/
static void StdinProc(clientData, mask)
ClientData clientData;		/* Not used. */
int mask;			/* Not used. */
{
	int 	code,
		count;
	char 	*cmd;

	Tcl_Channel newchan, chan = (Tcl_Channel) clientData;

	count = Tcl_Gets(chan, &line);

	if (count < 0) {
		if (!gotPartial) {
			if (tty) {
				Tcl_Exit(0);
			} else {
				Tcl_DeleteChannelHandler(chan, StdinProc,
							 (ClientData)
							 chan);
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

	/* Disable the stdin channel handler while evaluating the command;
	   otherwise if the command re-enters the event loop we might
	   process commands from stdin before the current command is
	   finished.  Among other things, this will trash the text of the
	   command being evaluated. */

	Tcl_CreateChannelHandler(chan, TCL_ACTIVE, StdinProc,
				 (ClientData) chan);
	code = Tcl_RecordAndEval(interp, cmd, TCL_EVAL_GLOBAL);
	newchan = Tcl_GetChannel(interp, "stdin", NULL);
	if (chan != newchan) {
		Tcl_DeleteChannelHandler(chan, StdinProc,
					 (ClientData) chan);
	}
	if (newchan) {
		Tcl_CreateChannelHandler(newchan,
					 TCL_READABLE | TCL_ACTIVE,
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

	/* Output a prompt. */

      prompt:
	if (tty) {
		Prompt(interp, gotPartial);
	}
	Tcl_ResetResult(interp);
}



/***********************************************************************
 *
 * Function:    Prompt
 *
 * Summary:     Issue a prompt on standard output, or invoke a script
 *		to issue the prompt
 *
 * Parameters:  None
 *
 * Returns:     Outputs a prompt, Tcl script may be evaluated in interp
 *
 ***********************************************************************/
static void Prompt(interp, partial)
Tcl_Interp *interp;		/* Interpreter to use for prompting. */

int partial;			/* Non-zero means there already exists a
				   partial command, so use the secondary
				   prompt. */
{
	char 	*promptCmd;
	int 	code;
	
	Tcl_Channel outChannel, errChannel;

	errChannel 	= Tcl_GetChannel(interp, "stderr", NULL);
	promptCmd 	= Tcl_GetVar(interp,
			       partial ? "tcl_prompt2" : "tcl_prompt1",
			       TCL_GLOBAL_ONLY);
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
			/* We must check that errChannel is a real channel -
			   it is possible that someone has transferred
			   stderr out of this interpreter with "interp
			   transfer". */

			errChannel =
			    Tcl_GetChannel(interp, "stdout", NULL);
			if (errChannel != (Tcl_Channel) NULL) {
				Tcl_Write(errChannel, interp->result, -1);
				Tcl_Write(errChannel, "\n", 1);
			}
			goto defaultPrompt;
		} else if (*interp->result && outChannel) {
			Tcl_Write(outChannel, interp->result,
				  strlen(interp->result));
		}
	}
	if (outChannel) {
		Tcl_Flush(outChannel);
	}
}

#endif				/* !HAVE_READLINE_EXTRA */

/***********************************************************************
 *
 * Function:    display
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
void display(char *text, char *tag, int type)
{
	int 	i;

#ifdef TK
	if (usetk) {
		Tcl_DString disp;

		Tcl_DStringInit(&disp);
		if (mode == 0) {
			Tcl_DStringAppend(&disp,
					  ".f.t mark set display {insert linestart};",
					  -1);
			mode = 1;
		}
		Tcl_DStringAppend(&disp, ".f.t insert display", -1);
		Tcl_DStringAppendElement(&disp, text);
		if (tag)
			Tcl_DStringAppendElement(&disp, tag);
		if (strlen(text) && (text[strlen(text) - 1] == '\n')) {
			mode = 0;
		}
		if (mode == 0)
			Tcl_DStringAppend(&disp,
					  ";.f.t mark unset display", -1);
		/* printf("Exec |%s|\n", Tcl_DStringValue(&disp)); */
		Tcl_Eval(interp, Tcl_DStringValue(&disp));
		/* puts(interp->result); */
		Tcl_DStringFree(&disp);
		return;
	}
#endif
	type++;

	for (i = 0; i < strlen(text); i++) {
		if (mode == 0) {
			/* At prompt */
			/* Dumb hack to erase prompt */
			printf
			    ("\r                                               \r");
			mode = -1;	/* Beginning of line */
		}
		if (mode != type) {
			if (mode != -1)
				printf("\n");	/* end current line */
			printf("%s", tag);
			mode = type;
		}
		printf("%c", text[i]);
		if (text[i] == '\n') {
			mode = 0;
#ifdef HAVE_READLINE_EXTRA
			rl_forced_update_display();	/* Bring the prompt back */
#else
			Prompt(interp, gotPartial);
#endif
		}
	}
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
