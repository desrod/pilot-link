
/*
 * Portions of this file are copied or derivered from various files in the
 * tcl8.0b1 distribution. 
 * Those portions are Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/* The remainder of this file is Copyright (c) 1997, Kenneth Albanowski, and
 * is free software, licensed under the GNU Public License V2.  See the file
 * COPYING for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <tcl.h>
#include <pi-source.h>
#include <pi-socket.h>
#include <pi-dlp.h>

Tcl_Namespace *
Tcl_CreateNamespace(Tcl_Interp *interp, char *name, ClientData clientData, Tcl_NamespaceDeleteProc *deleteProc);
Tcl_Namespace *
Tcl_FindNamespace(Tcl_Interp *interp, char *name, Tcl_Namespace *contextNsPtr, int flags);
int
Tcl_PushCallFrame(Tcl_Interp *interp, Tcl_CallFrame *callFramePtr, Tcl_Namespace *namespacePtr, int isProcCallFrame);
void
Tcl_PopCallFrame(Tcl_Interp *interp);

struct tcl_e {char *name; long value;};

char buf[0xffff];

static struct tcl_e domains[] = {
	{"PI_AF_SLP", PI_AF_SLP},
	{"AF_SLP", PI_AF_SLP},
	{"SLP", PI_AF_SLP},
	{0,0}};
static struct tcl_e protocols[] = {
	{"PI_PF_SLP", PI_PF_SLP}, 
	{"PI_PF_PADP", PI_PF_PADP},
	{"PI_PF_LOOP", PI_PF_LOOP},
	{"PF_SLP", PI_PF_SLP}, 
	{"PF_PADP", PI_PF_PADP},
	{"PF_LOOP", PI_PF_LOOP},
	{"SLP", PI_PF_SLP}, 
	{"PADP", PI_PF_PADP},
	{"LOOP", PI_PF_LOOP},
	{0,0}};
static struct tcl_e types[] = {
	{"PI_SOCK_STREAM", PI_SOCK_STREAM},
	{"PI_SOCK_DGRAM", PI_SOCK_DGRAM},
	{"PI_SOCK_RAW", PI_SOCK_RAW},
	{"PI_SOCK_SEQPACKET", PI_SOCK_SEQPACKET},
	{"SOCK_STREAM", PI_SOCK_STREAM},
	{"SOCK_DGRAM", PI_SOCK_DGRAM},
	{"SOCK_RAW", PI_SOCK_RAW},
	{"SOCK_SEQPACKET", PI_SOCK_SEQPACKET},
	{"STREAM", PI_SOCK_STREAM},
	{"DGRAM", PI_SOCK_DGRAM},
	{"RAW", PI_SOCK_RAW},
	{"SEQPACKET", PI_SOCK_SEQPACKET},
	{0,0}};

static struct tcl_e ports[] = {
	{"DLP", PI_PilotSocketDLP},
	{"Console", PI_PilotSocketConsole},
	{"Debugger", PI_PilotSocketDebugger},
	{"RemoteUI", PI_PilotSocketRemoteUI},
	{0,0}};

static int
PisockCloseProc(ClientData instanceData, Tcl_Interp* interp);
static int
PisockInputProc(ClientData instanceData, char *buf, int bufSize, int *errorCodePtr);
static int
PisockOutputProc(ClientData instanceData, char *buf, int toWrite, int *errorCodePtr);
static void
PisockWatchProc(ClientData instanceData, int mask);
static int
PisockGetHandleProc(ClientData instanceData, int direction, ClientData *handlePtr);


static Tcl_ChannelType pisockChannelType = {
    "pisock",                              /* Type name. */
    NULL/*PisockBlockModeProc*/,                   /* Set blocking/nonblocking mode.*/
    PisockCloseProc,                       /* Close proc. */
    PisockInputProc,                       /* Input proc. */
    PisockOutputProc,                      /* Output proc. */
    NULL,                               /* Seek proc. */
    NULL,                               /* Set option proc. */
    NULL,                   /* Get option proc. */
    PisockWatchProc,                       /* Initialize notifier. */
    PisockGetHandleProc,                   /* Get OS handles out of channel. */
};

typedef void (Tcl_PisockAcceptProc)(ClientData,Tcl_Channel);


typedef struct PisockState {
    Tcl_Channel channel;        /* Channel associated with this file. */
    int fd;                     /* The socket itself. */
    int flags;                  /* ORed combination of the bitfields
                                 * defined below. */

    Tcl_PisockAcceptProc *acceptProc;
                                    /* Proc to call on accept. */
    ClientData acceptProcData;  /* The data for the accept proc. */
} PisockState;

#define TCP_ASYNC_SOCKET        (1<<0)  /* Asynchronous socket. */
#define TCP_ASYNC_CONNECT       (1<<1)  /* Async connect in progress. */

static int
WaitForConnect(PisockState * statePtr, int * errorCodePtr);

typedef struct AcceptCallback {
    char *script;                       /* Script to invoke. */
    Tcl_Interp *interp;                 /* Interpreter in which to run it. */
} AcceptCallback;


static void     AcceptCallbackProc _ANSI_ARGS_((ClientData callbackData,
                    Tcl_Channel chan));
static void     RegisterPisockServerInterpCleanup _ANSI_ARGS_((Tcl_Interp *interp,
                    AcceptCallback *acceptCallbackPtr));
static void     PisockAcceptCallbacksDeleteProc _ANSI_ARGS_((
                    ClientData clientData, Tcl_Interp *interp));
static void     PisockServerCloseProc _ANSI_ARGS_((ClientData callbackData));
static void     UnregisterPisockServerInterpCleanupProc _ANSI_ARGS_((
                    Tcl_Interp *interp, AcceptCallback *acceptCallbackPtr));

        /* ARGSUSED */
static void
PisockAcceptCallbacksDeleteProc(clientData, interp)
    ClientData clientData;      /* Data which was passed when the assocdata
                                 * was registered. */
    Tcl_Interp *interp;         /* Interpreter being deleted - not used. */
{
    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSearch;
    AcceptCallback *acceptCallbackPtr;

    hTblPtr = (Tcl_HashTable *) clientData;
    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
             hPtr != (Tcl_HashEntry *) NULL;
             hPtr = Tcl_NextHashEntry(&hSearch)) {
        acceptCallbackPtr = (AcceptCallback *) Tcl_GetHashValue(hPtr);
        acceptCallbackPtr->interp = (Tcl_Interp *) NULL;
    }
    Tcl_DeleteHashTable(hTblPtr);
    ckfree((char *) hTblPtr);
}

static void
RegisterPisockServerInterpCleanup(interp, acceptCallbackPtr)
    Tcl_Interp *interp;         /* Interpreter for which we want to be
                                 * informed of deletion. */
    AcceptCallback *acceptCallbackPtr;
                                /* The accept callback record whose
                                 * interp field we want set to NULL when
                                 * the interpreter is deleted. */
{
    Tcl_HashTable *hTblPtr;     /* Hash table for accept callback
                                 * records to smash when the interpreter
                                 * will be deleted. */
    Tcl_HashEntry *hPtr;        /* Entry for this record. */
    int new;                    /* Is the entry new? */

    hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp,
            "pisockAcceptCallbacks",
            NULL);
    if (hTblPtr == (Tcl_HashTable *) NULL) {
        hTblPtr = (Tcl_HashTable *) ckalloc((unsigned) sizeof(Tcl_HashTable));
        Tcl_InitHashTable(hTblPtr, TCL_ONE_WORD_KEYS);
        (void) Tcl_SetAssocData(interp, "pisockAcceptCallbacks",
                PisockAcceptCallbacksDeleteProc, (ClientData) hTblPtr);
    }
    hPtr = Tcl_CreateHashEntry(hTblPtr, (char *) acceptCallbackPtr, &new);
    if (!new) {
        panic("RegisterPisockServerCleanup: damaged accept record table");
    }
    Tcl_SetHashValue(hPtr, (ClientData) acceptCallbackPtr);
}

static void
UnregisterPisockServerInterpCleanupProc(interp, acceptCallbackPtr)
    Tcl_Interp *interp;         /* Interpreter in which the accept callback
                                 * record was registered. */
    AcceptCallback *acceptCallbackPtr;
                                /* The record for which to delete the
                                 * registration. */
{
    Tcl_HashTable *hTblPtr;
    Tcl_HashEntry *hPtr;

    hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp,
            "pisockAcceptCallbacks", NULL);
    if (hTblPtr == (Tcl_HashTable *) NULL) {
        return;
    }
    hPtr = Tcl_FindHashEntry(hTblPtr, (char *) acceptCallbackPtr);
    if (hPtr == (Tcl_HashEntry *) NULL) {
        return;
    }
    Tcl_DeleteHashEntry(hPtr);
}


        /* ARGSUSED */
static int
PisockCloseProc(instanceData, interp)
    ClientData instanceData;    /* The socket to close. */
    Tcl_Interp *interp;         /* For error reporting - unused. */
{
    PisockState *statePtr = (PisockState *) instanceData;
    int errorCode = 0;

    /*
     * Delete a file handler that may be active for this socket if this
     * is a server socket - the file handler was created automatically
     * by Tcl as part of the mechanism to accept new client connections.
     * Channel handlers are already deleted in the generic IO channel
     * closing code that called this function, so we do not have to
     * delete them here.
     */

    Tcl_DeleteFileHandler(statePtr->fd);

    if (pi_close(statePtr->fd) < 0) {
        errorCode = errno;
    }
    ckfree((char *) statePtr);

    return errorCode;
}

        /* ARGSUSED */
static int
PisockInputProc(instanceData, buf, bufSize, errorCodePtr)
    ClientData instanceData;            /* Socket state. */
    char *buf;                          /* Where to store data read. */
    int bufSize;                        /* How much space is available
                                         * in the buffer? */
    int *errorCodePtr;                  /* Where to store error code. */
{
    PisockState *statePtr = (PisockState *) instanceData;
    int bytesRead, state;

    *errorCodePtr = 0;
    state = WaitForConnect(statePtr, errorCodePtr);
    if (state != 0) {
        return -1;
    }
    bytesRead = pi_recv(statePtr->fd, buf, bufSize, 0);
    if (bytesRead > -1) {
        return bytesRead;
    }
    if (errno == ECONNRESET) {

        /*
         * Turn ECONNRESET into a soft EOF condition.
         */

        return 0;
    }
    *errorCodePtr = errno;
    return -1;
}

static int
PisockOutputProc(instanceData, buf, toWrite, errorCodePtr)
    ClientData instanceData;            /* Socket state. */
    char *buf;                          /* The data buffer. */
    int toWrite;                        /* How many bytes to write? */
    int *errorCodePtr;                  /* Where to store error code. */
{
    PisockState *statePtr = (PisockState *) instanceData;
    int written;
    int state;                          /* Of waiting for connection. */

    *errorCodePtr = 0;
    state = WaitForConnect(statePtr, errorCodePtr);
    if (state != 0) {
        return -1;
    }
    written = pi_send(statePtr->fd, buf, toWrite, 0);
    if (written > -1) {
        return written;
    }
    *errorCodePtr = errno;
    return -1;
}

static void
PisockWatchProc(instanceData, mask)
    ClientData instanceData;            /* The socket state. */
    int mask;                           /* Events of interest; an OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    PisockState *statePtr = (PisockState *) instanceData;

    if (mask) {
        Tcl_CreateFileHandler(statePtr->fd, mask,
                (Tcl_FileProc *) Tcl_NotifyChannel,
                (ClientData) statePtr->channel);
    } else {
        Tcl_DeleteFileHandler(statePtr->fd);
    }
}

        /* ARGSUSED */
static int
PisockGetHandleProc(instanceData, direction, handlePtr)
    ClientData instanceData;    /* The socket state. */
    int direction;              /* Not used. */
    ClientData *handlePtr;      /* Where to store the handle.  */
{
    PisockState *statePtr = (PisockState *) instanceData;

    *handlePtr = (ClientData)statePtr->fd;
    return TCL_OK;
}


static int
WaitForConnect(statePtr, errorCodePtr)
    PisockState *statePtr;         /* State of the socket. */
    int *errorCodePtr;          /* Where to store errors? */
{
    int timeOut;                /* How long to wait. */
    int state;                  /* Of calling TclWaitForFile. */
    int flags;                  /* fcntl flags for the socket. */

    /*
     * If an asynchronous connect is in progress, attempt to wait for it
     * to complete before reading.
     */

#if 0
    if (statePtr->flags & TCP_ASYNC_CONNECT) {
        if (statePtr->flags & TCP_ASYNC_SOCKET) {
            timeOut = 0;
        } else {
            timeOut = -1;
        }
        errno = 0;
        state = TclUnixWaitForFile(statePtr->fd,
                TCL_WRITABLE | TCL_EXCEPTION, timeOut);
        if (!(statePtr->flags & TCP_ASYNC_SOCKET)) {
#ifndef USE_FIONBIO
            flags = fcntl(statePtr->fd, F_GETFL);
            flags &= (~(O_NONBLOCK));
            (void) fcntl(statePtr->fd, F_SETFL, flags);
#endif

#ifdef  USE_FIONBIO
            flags = 0;
            (void) ioctl(statePtr->fd, FIONBIO, &flags);
#endif
        }
        if (state & TCL_EXCEPTION) {
            return -1;
        }
        if (state & TCL_WRITABLE) {
            statePtr->flags &= (~(TCP_ASYNC_CONNECT));
        } else if (timeOut == 0) {
            *errorCodePtr = errno = EWOULDBLOCK;
            return -1;
        }
    }
#endif
    return 0;
}

static void
AcceptCallbackProc(callbackData, chan)
    ClientData callbackData;            /* The data stored when the callback
                                         * was created in the call to
                                         * Tcl_OpenTcpServer. */
    Tcl_Channel chan;                   /* Channel for the newly accepted
                                         * connection. */
{
    AcceptCallback *acceptCallbackPtr;
    Tcl_Interp *interp;
    char *script;
    char portBuf[10];
    int result;

    acceptCallbackPtr = (AcceptCallback *) callbackData;

    /*
     * Check if the callback is still valid; the interpreter may have gone
     * away, this is signalled by setting the interp field of the callback
     * data to NULL.
     */

    if (acceptCallbackPtr->interp != (Tcl_Interp *) NULL) {

        script = acceptCallbackPtr->script;
        interp = acceptCallbackPtr->interp;

        Tcl_Preserve((ClientData) script);
        Tcl_Preserve((ClientData) interp);

        Tcl_RegisterChannel(interp, chan);
        result = Tcl_VarEval(interp, script, " ", Tcl_GetChannelName(chan),
                (char *) NULL);
        if (result != TCL_OK) {
            Tcl_BackgroundError(interp);
            Tcl_UnregisterChannel(interp, chan);
        }
        Tcl_Release((ClientData) interp);
        Tcl_Release((ClientData) script);
    } else {

        /*
         * The interpreter has been deleted, so there is no useful
         * way to utilize the client socket - just close it.
         */

        Tcl_Close((Tcl_Interp *) NULL, chan);
    }
}

static void
PisockServerCloseProc(callbackData)
    ClientData callbackData;    /* The data passed in the call to
                                 * Tcl_CreateCloseHandler. */
{
    AcceptCallback *acceptCallbackPtr;
                                /* The actual data. */

    acceptCallbackPtr = (AcceptCallback *) callbackData;
    if (acceptCallbackPtr->interp != (Tcl_Interp *) NULL) {
        UnregisterPisockServerInterpCleanupProc(acceptCallbackPtr->interp,
                acceptCallbackPtr);
    }
    Tcl_EventuallyFree((ClientData) acceptCallbackPtr->script, TCL_DYNAMIC);
    ckfree((char *) acceptCallbackPtr);
}


static void
PisockAccept(data, mask)
    ClientData data;                    /* Callback token. */
    int mask;                           /* Not used. */
{
    PisockState *sockState;                /* Client data of server socket. */
    int newsock;                        /* The new client socket */
    PisockState *newSockState;             /* State for new socket. */
    int len;                            /* For accept interface */
    char channelName[20];

    sockState = (PisockState *) data;

    newsock = pi_accept(sockState->fd, 0, 0);
    if (newsock < 0) {
        return;
    }

    /*
     * Set close-on-exec flag to prevent the newly accepted socket from
     * being inherited by child processes.
     */

    /*(void) fcntl(newsock, F_SETFD, FD_CLOEXEC);*/

    newSockState = (PisockState *) ckalloc((unsigned) sizeof(PisockState));

    newSockState->flags = 0;
    newSockState->fd = newsock;
    newSockState->acceptProc = (Tcl_PisockAcceptProc *) NULL;
    newSockState->acceptProcData = (ClientData) NULL;

    sprintf(channelName, "pisock%d", newsock);
    newSockState->channel = Tcl_CreateChannel(&pisockChannelType, channelName,
            (ClientData) newSockState, (TCL_READABLE | TCL_WRITABLE));

    /*Tcl_SetChannelOption((Tcl_Interp *) NULL, newSockState->channel,
            "-translation", "auto crlf");*/

    if (sockState->acceptProc != (Tcl_PisockAcceptProc *) NULL) {
        (sockState->acceptProc) (sockState->acceptProcData,
                newSockState->channel);
    }
}



static long tcl_enum(Tcl_Interp *interp, Tcl_Obj *object, struct tcl_e * e)
{
	int i;
	long result;
	char * str = Tcl_GetStringFromObj(object, 0);
	for(i=0;e[i].name;i++) {
		if (strcmp(e[i].name, str)==0)
			return e[i].value;
	}
	result = 0;
	Tcl_GetLongFromObj(interp, object, &result);
	return result;
}

static int tcl_socket(Tcl_Interp *interp, Tcl_Obj *object)
{
	int fd= 0;
	Tcl_Channel c = Tcl_GetChannel(interp, Tcl_GetStringFromObj(object,0), 0);
	if (!c) {
		Tcl_GetIntFromObj(interp, object, &fd);
	} else {
		Tcl_GetChannelHandle(c, TCL_WRITABLE, (ClientData)&fd);
	}
	return fd;
}

PisockState *
CreateSocket(Tcl_Interp * interp, int protocol, char * remote, int server)
{
	PisockState *statePtr;
	
	int result;
	int sock;
	int type;
	int domain = PI_AF_SLP;
	char channelName[20];
	struct pi_sockaddr * addr = 0;
	int alen = 0;
	
	statePtr = (PisockState *) ckalloc((unsigned) sizeof(PisockState));
	statePtr->flags = 0;

	if (protocol == PI_PF_SLP) 
		type = PI_SOCK_RAW;
	else
		type = PI_SOCK_STREAM;
	
	sock = pi_socket(domain, type, protocol);
	printf("Called pi_socket\n");

	statePtr->fd = sock;

	if (domain == PI_AF_SLP) {
		char * device;
		device = remote;
		alen = strlen(device)+1+4;
		addr = (struct pi_sockaddr*)malloc(alen);
		strcpy(addr->pi_device, device);
		addr->pi_port = 3;
		addr->pi_family = PI_AF_SLP;
	}
	printf("addr = %d\n", (long)addr);

	if (server) {
		result = pi_bind(sock, addr, alen);
		pi_listen(sock, 1);
	} else {
      result = pi_connect(sock, addr, alen);
	}
	printf(" result = %d\n", result);

   return statePtr;
}

static Tcl_Channel
ClientSocket(Tcl_Interp * interp, int protocol, char * remote)
{
	PisockState *statePtr;
	
	int result;
	int sock;
	int type;
	char channelName[20];
	
	statePtr = CreateSocket(interp, protocol, remote, 0);
   if (statePtr == NULL) {
        return NULL;
   }
   printf("Created socket\n");
                
	statePtr->acceptProc = NULL;
	statePtr->acceptProcData = (ClientData) NULL;

    sprintf(channelName, "pisock%d", statePtr->fd);
    statePtr->channel = Tcl_CreateChannel(&pisockChannelType, channelName,
                (ClientData) statePtr, (TCL_READABLE|TCL_WRITABLE));
    return statePtr->channel;
}

static Tcl_Channel
ServerSocket(Tcl_Interp * interp, int protocol, char * remote,
	Tcl_PisockAcceptProc *acceptProc, ClientData acceptProcData)
{
	PisockState *statePtr;
	
	int result;
	int sock;
	int type;
	char channelName[20];
	
	statePtr = CreateSocket(interp, protocol, remote, 1);
   if (statePtr == NULL) {
        return NULL;
   }
   printf("Server: Created socket\n");
                
	statePtr->acceptProc = acceptProc;
	statePtr->acceptProcData = (ClientData)acceptProcData;
   Tcl_CreateFileHandler(statePtr->fd, TCL_READABLE, PisockAccept,
           (ClientData) statePtr);

    sprintf(channelName, "pisock%d", statePtr->fd);
    statePtr->channel = Tcl_CreateChannel(&pisockChannelType, channelName,
                (ClientData) statePtr, 0);
    
    printf("Competed bounding\n");
    return statePtr->channel;
}

int
OpenSocketCmd(notUsed, interp, argc, argv)
    ClientData notUsed;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    int a, server, port;
    char *arg, *copyScript, *host, *script;
    char *myaddr = NULL;
    int protocol = PI_PF_PADP;
    int async = 0;
    Tcl_Channel chan;
    AcceptCallback *acceptCallbackPtr;
    
    server = 0;
    script = NULL;

	for (a = 1; a < argc; a++) {
		arg = argv[a];
		if (arg[0] == '-') {
			if (strcmp(arg, "-server") == 0) {
         	if (async == 1) {
            	Tcl_AppendResult(interp,
                            "cannot set -async option for server sockets",
                            (char *) NULL);
                    return TCL_ERROR;
                }
		server = 1;
		a++;
		if (a >= argc) {
		    Tcl_AppendResult(interp,
			    "no argument given for -server option",
                            (char *) NULL);
		    return TCL_ERROR;
		}
                script = argv[a];
            }  else if (strcmp(arg, "-myprotocol") == 0) {
		a++;
                if (a >= argc) {
		    Tcl_AppendResult(interp,
			    "no argument given for -myprotocol option",
                            (char *) NULL);
		    return TCL_ERROR;
		}
		protocol = atoi(argv[a]);
			}/*else if (strcmp(arg, "-async") == 0) {
                if (server == 1) {
                    Tcl_AppendResult(interp,
                            "cannot set -async option for server sockets",
                            (char *) NULL);
                    return TCL_ERROR;
                }
                async = 1;
	    }*/ else {
		Tcl_AppendResult(interp, "bad option \"", arg,
                        "\", must be -myprotocol or -server",
                        (char *) NULL);
		return TCL_ERROR;
	    }
	} else {
	    break;
	}
    }
#if 0
    if (server) {
        host = myaddr;		/* NULL implies INADDR_ANY */
	if (myport != 0) {
	    Tcl_AppendResult(interp, "Option -myport is not valid for servers",
		    NULL);
	    return TCL_ERROR;
	}
    } else 
#endif
    if (a < argc) {
	host = argv[a];
	a++;
    } else {
wrongNumArgs:
	Tcl_AppendResult(interp, "wrong # args: should be either:\n",
		argv[0],
                " ?-myprotocol protocol? ?-async? host\n",
		argv[0],
                " -server command ?-myprotocol protocol? host",
                (char *) NULL);
        return TCL_ERROR;
    }

    if (server) {
        acceptCallbackPtr = (AcceptCallback *) ckalloc((unsigned)
                sizeof(AcceptCallback));
        copyScript = ckalloc((unsigned) strlen(script) + 1);
        strcpy(copyScript, script);
        acceptCallbackPtr->script = copyScript;
        acceptCallbackPtr->interp = interp;
        chan = ServerSocket(interp, protocol, host, AcceptCallbackProc,
                (ClientData) acceptCallbackPtr);
        if (chan == (Tcl_Channel) NULL) {
            ckfree(copyScript);
            ckfree((char *) acceptCallbackPtr);
            return TCL_ERROR;
        }

        /*
         * Register with the interpreter to let us know when the
         * interpreter is deleted (by having the callback set the
         * acceptCallbackPtr->interp field to NULL). This is to
         * avoid trying to eval the script in a deleted interpreter.
         */

        RegisterPisockServerInterpCleanup(interp, acceptCallbackPtr);
        
        /*
         * Register a close callback. This callback will inform the
         * interpreter (if it still exists) that this channel does not
         * need to be informed when the interpreter is deleted.
         */
        
        Tcl_CreateCloseHandler(chan, PisockServerCloseProc,
                (ClientData) acceptCallbackPtr);
    } else {
    		printf("Protocol = %d, host = '%s'\n", protocol, host);
        chan = ClientSocket(interp, protocol, host);
        if (chan == (Tcl_Channel) NULL) {
            return TCL_ERROR;
        }
        printf("got channel!\n");
    }
    Tcl_RegisterChannel(interp, chan);            
    Tcl_AppendResult(interp, Tcl_GetChannelName(chan), (char *) NULL);
    
    return TCL_OK;
}

static int
socketCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	PisockState *statePtr;
	
	int result;
	int x,y,z;
	char channelName[40];
	if (objc != 4) {
		Tcl_SetResult(interp, "Usage: x, y, z", TCL_STATIC);
		return TCL_ERROR;
	}
	
	x = tcl_enum(interp, objv[1], domains);
	y = tcl_enum(interp, objv[2], types);
	z = tcl_enum(interp, objv[3], protocols);
	
	result = pi_socket(x,y,z);

    statePtr = (PisockState *) ckalloc((unsigned) sizeof(PisockState));
    statePtr->flags = 0;
    statePtr->fd = result;

    /*statePtr = CreateSocket(interp, port, host, 0, myaddr, myport, async);
    if (statePtr == NULL) {
        return NULL;
    }*/

    /*statePtr->acceptProc = NULL;
    statePtr->acceptProcData = (ClientData) NULL;
    Tcl_CreateFileHandler(statePtr->fd, TCL_READABLE, TcpAccept,
            (ClientData) statePtr);*/

    sprintf(channelName, "pisock%d", statePtr->fd);

    statePtr->channel = Tcl_CreateChannel(&pisockChannelType, channelName,
            (ClientData) statePtr, (TCL_READABLE | TCL_WRITABLE));
	    
    /*return statePtr;*/

    Tcl_RegisterChannel(interp, statePtr->channel);
    Tcl_AppendResult(interp, Tcl_GetChannelName(statePtr->channel), (char *) NULL);
    return TCL_OK;
/*            
	
	if (result == -1) {
		Tcl_SetResult(interp, Tcl_PosixError(interp), TCL_STATIC);
		return TCL_ERROR;
	}
	else {
		Tcl_SetIntObj(Tcl_GetObjResult(interp), result);
		return TCL_OK;
	}*/
}

static int
bindCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	int result;
	int s;
	int port, family;
	char * device;
	struct pi_sockaddr * addr = 0;
	int alen = 0;
	if (objc != 5) {
		Tcl_SetResult(interp, "Usage: socket family device port", TCL_STATIC);
		return TCL_ERROR;
	}
	
	Tcl_GetIntFromObj(interp, objv[1], &s);
	family = tcl_enum(interp, objv[2], domains);
	
	if (family == PI_AF_SLP) {
		device = Tcl_GetStringFromObj(objv[3], 0);
		port = tcl_enum(interp, objv[4], ports);
		alen = strlen(device)+1+4;
		addr = (struct pi_sockaddr*)malloc(alen);
		strcpy(addr->pi_device, device);
		addr->pi_port = port;
		addr->pi_family = family;
	}
	
	result = pi_bind(s, addr, alen);
	
	if (addr)
		free(addr);
	
	if (result == -1) {
		Tcl_SetResult(interp, Tcl_PosixError(interp), TCL_STATIC);
		return TCL_ERROR;
	}
	else {
		Tcl_SetIntObj(Tcl_GetObjResult(interp), result);
		return TCL_OK;
	}
}

static int
OpenConduitCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	if (objc != 2) {
		Tcl_SetResult(interp, "Usage: Status socket", TCL_STATIC);
		return TCL_ERROR;
	}
	dlp_OpenConduit(tcl_socket(interp, objv[1]));
	return TCL_OK;
}

static int
OpenDBCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	int result;
	int handle;
	int cardno=0,mode=0x80|0x40;
	char * name;
	if ((objc < 3 )|| (objc > 5)) {
		Tcl_SetResult(interp, "Usage: Open socket name [card [mode]]", TCL_STATIC);
		return TCL_ERROR;
	}
	if (objc>=3)
		Tcl_GetIntFromObj(interp, objv[3], &cardno);
	if (objc>=4) {
		char * modestr = Tcl_GetStringFromObj(objv[4],0);
		mode = 0;
		if (strchr(modestr, 'S') || strchr(modestr,'s'))
			mode |= dlpOpenSecret;
		if (strchr(modestr, 'E') || strchr(modestr,'e'))
			mode |= dlpOpenExclusive;
		if (strchr(modestr, 'W') || strchr(modestr,'w'))
			mode |= dlpOpenWrite;
		if (strchr(modestr, 'R') || strchr(modestr,'r'))
			mode |= dlpOpenRead;
	}
	name = Tcl_GetStringFromObj(objv[2],0);
	result = dlp_OpenDB(tcl_socket(interp, objv[1]), cardno, mode, name, &handle);
	printf("Result = %d, name = '%s', mode = %x, card = %d, handle = %d\n", 
		result, name, mode, cardno, handle);
	
	if (result<0) {
		Tcl_SetResult(interp, dlp_strerror(result), TCL_STATIC);
		return TCL_ERROR;
	}
	
	Tcl_SetIntObj(Tcl_GetObjResult(interp), handle);
	return TCL_OK;
}

static int
GetRecordCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	int result;
	int handle;
	int index;
	int len,attr,cat;
	recordid_t id;
	Tcl_Obj * newObj[5];
	
	if (objc != 4 ) {
		Tcl_SetResult(interp, "Usage: GetRecord socket db index", TCL_STATIC);
		return TCL_ERROR;
	}
	
	Tcl_GetIntFromObj(interp, objv[3], &index);
	Tcl_GetIntFromObj(interp, objv[2], &handle);
	result = dlp_ReadRecordByIndex(tcl_socket(interp, objv[1]), handle, index, buf, &id, &len, &attr, &cat);
	
	if (result<0) {
		Tcl_SetResult(interp, dlp_strerror(result), TCL_STATIC);
		return TCL_ERROR;
	}
	
	newObj[0] = Tcl_NewStringObj(buf, len);
	newObj[1] = Tcl_NewIntObj(id);
	newObj[2] = Tcl_NewIntObj(index);
	newObj[3] = Tcl_NewIntObj(attr);
	newObj[4] = Tcl_NewIntObj(cat);
	
	Tcl_SetListObj(Tcl_GetObjResult(interp), 5, newObj);
	return TCL_OK;
}

static int
AddSyncLogEntryCmd(ClientData clientData, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
	if (objc != 3) {
		Tcl_SetResult(interp, "Usage: Log socket text", TCL_STATIC);
		return TCL_ERROR;
	}
	dlp_AddSyncLogEntry(tcl_socket(interp, objv[1]), Tcl_GetStringFromObj(objv[2],0));
	return TCL_OK;
}

static struct { char * name; Tcl_ObjCmdProc *proc; int export;} doprocs[] = {
	{"GetRecord", GetRecordCmd, 1},
	{"Open", OpenDBCmd, 1},
	{"Status", OpenConduitCmd, 1},
	{"Log", AddSyncLogEntryCmd, 1},
	{0,0}
};

static struct { char * name; Tcl_CmdProc *proc; int export;} procs[] = {
	{"Open", OpenSocketCmd, 1},
	{0,0}
};

static struct { char * name; Tcl_ObjCmdProc *proc; int export;} oprocs[] = {
	{"socket", socketCmd, 0},
	{"bind", bindCmd, 0},
	{0,0}
};

int Pilotlink_Init(Tcl_Interp *interp) {
	int i;
	Tcl_Namespace  * namespacePtr;
	Tcl_CallFrame frame;
	
	namespacePtr = Tcl_FindNamespace(interp, "Pi", 0, TCL_GLOBAL_ONLY);
	
	if (!namespacePtr)
	   namespacePtr = Tcl_CreateNamespace(interp, "Pi", (ClientData) NULL,
	                       (Tcl_NamespaceDeleteProc *) NULL);
	                      
	Tcl_PushCallFrame(interp, &frame, namespacePtr,
            /*isProcCallFrame*/ 0);

	for(i=0;procs[i].name;i++) {
		Tcl_CreateCommand(interp, procs[i].name, procs[i].proc, 0, 0);
		if (procs[i].export)
			Tcl_VarEval(interp, "namespace export ", procs[i].name, 0);
	}
	for(i=0;oprocs[i].name;i++) {
		Tcl_CreateObjCommand(interp, oprocs[i].name, oprocs[i].proc, 0, 0);
		if (oprocs[i].export)
			Tcl_VarEval(interp, "namespace export ", oprocs[i].name, 0);
	}
	Tcl_VarEval(interp, "namespace export Dlp", 0);
	
 Tcl_PopCallFrame(interp);

	namespacePtr = Tcl_FindNamespace(interp, "Pi::Dlp", 0, TCL_GLOBAL_ONLY);
	
	if (!namespacePtr)
	   namespacePtr = Tcl_CreateNamespace(interp, "Pi::Dlp", (ClientData) NULL,
	                       (Tcl_NamespaceDeleteProc *) NULL);
	                      
	Tcl_PushCallFrame(interp, &frame, namespacePtr,
            /*isProcCallFrame*/ 0);

	for(i=0;doprocs[i].name;i++) {
		Tcl_CreateObjCommand(interp, doprocs[i].name, doprocs[i].proc, 0, 0);
		if (doprocs[i].export)
			Tcl_VarEval(interp, "namespace export ", doprocs[i].name, 0);
	}
	
 Tcl_PopCallFrame(interp);

	return TCL_OK;
}
