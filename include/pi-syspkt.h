
#ifndef _PILOT_SYSPKT_H
#define _PILOT_SYSPKT_H

struct Pilot_registers {
  unsigned long A[7];
  unsigned long D[8];
  unsigned long USP, SSP;
  unsigned long PC, SR;
};

struct Pilot_breakpoint {
	unsigned long address;
	int enabled;
};

struct Pilot_state {
  struct Pilot_registers regs;
  int reset;
  int exception;
  int instructions[30];
  struct Pilot_breakpoint breakpoint[6];
  unsigned long func_start, func_end;
  char func_name[32];
  int trap_rev;
};

struct Pilot_continue {
  struct Pilot_registers regs;
  int watch;
  unsigned long watch_address;
  unsigned long watch_length;
  unsigned long watch_checksum;
};

struct RPC_param {
  int byRef;
  int size;
  int invert;
  int arg;
  void * data;
};

struct RPC_params {
  int trap;
  int reply;
  int args;
  struct RPC_param param[20];
};

extern int sys_RPCerror;

extern int sys_UnpackState(void * buffer, struct Pilot_state * s);

extern int sys_UnpackRegisters(void * buffer, struct Pilot_registers * r);

extern int syspkt_tx(struct pi_socket *ps, unsigned char *msg, int length);
                                    
extern int syspkt_rx(struct pi_socket *ps, unsigned char *buf, int len);
                                    

extern int sys_Continue(int sd, struct Pilot_continue * c);

extern int sys_QueryState(int sd);
extern int sys_ReadMemory(int sd, unsigned long addr, int len, void * buf);
extern int sys_WriteMemory(int sd, unsigned long addr, int len, void * buf);

extern int sys_SetBreakpoints(int sd, struct Pilot_breakpoint * b);

extern int sys_RemoteEvent(int sd, int penDown, int x, int y, int keypressed, 
                       int keymod, int keyasc, int keycode);

extern int sys_RPC(int sd, int socket, int trap, long * D0, long * A0, int params, struct RPC_param * param, int rep);

#define RPC_Short(data) (-2),((unsigned int)htons((data)))
#define RPC_Long(data) (-4),((unsigned int)htonl((data)))
#define RPC_Ptr(data,len) (len),((void*)(data)),0
#define RPC_LongPtr(ptr) (4),((void*)(ptr)),1
#define RPC_ShortPtr(ptr) (2),((void*)(ptr)),1
#define RPC_LongRef(ref) (4),((void*)(&(ref))),1
#define RPC_ShortRef(ref) (2),((void*)(&(ref))),1
#define RPC_End 0

#define RPC_IntReply  2
#define RPC_PtrReply  1
#define RPC_NoReply 0

extern int RPC(int sd, int socket, int trap, int ret, ...);

extern int PackRPC(struct RPC_params * p, int trap, int reply, ...);

extern unsigned long DoRPC(int sd, int socket, struct RPC_params * p, int * error);

extern int dlp_ProcessRPC(int sd, int trap, int ret, ...);

extern int RPC_Int_Void(int sd, int trap);
extern int RPC_Ptr_Void(int sd, int trap);

#endif /*_PILOT_SYSPKT_H_*/
