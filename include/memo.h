#ifndef _PILOT_MEMO_H_
#define _PILOT_MEMO_H_

struct Memo {
   char * text;
};

void free_Memo(struct Memo *);
void unpack_Memo(struct Memo *, unsigned char * record, int len);
void pack_Memo(struct Memo *, unsigned char * record, int * len);


#endif /* _PILOT_MEMO_H_ */
