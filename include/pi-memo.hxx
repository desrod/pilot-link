
#include "pi-appinfo.hxx"

const int MEMO_APP_INFO_SIZE = BASE_APP_INFO_SIZE;

struct memoAppInfo_t : public appInfo_t 
{
     memoAppInfo_t(void *buffer) : appInfo_t(buffer) { }
     
     void *pack(void) 
     {
	  unsigned char *ret = new unsigned char [MEMO_APP_INFO_SIZE];
	  baseAppInfoPack(ret);
	  return ret;
     }
};

class memoList_t;		// Forward declaration for older compilers

class memo_t : public baseApp_t
{
     friend class memoList_t;
     
     char *_text;
     int _size;
     
     memo_t *_next;
	  
     void *internalPack(unsigned char *);

   public:
     memo_t(void) : baseApp_t() { _text = NULL; _size = 0; }
     memo_t(void *buf) : baseApp_t() { unpack(buf); }
     memo_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
     {
	       unpack(buf);
     }
     memo_t(const memo_t &);

     void unpack(void *);
     ~memo_t() { if (_text) delete _text; }

     void *pack(int *i);
     void *pack(void *, int *);

     const char *text(void) const { return _text; }
};

class memoList_t 
{
     memo_t *_head;
     
   public:
     memoList_t(void) : _head(NULL) { }
     ~memoList_t();
     
     memo_t *first() { return _head; }
     memo_t *next(memo_t *ptr) { return ptr->_next; }

     void merge(memo_t &);
     void merge(memoList_t &);
};
     
