
#include "pi-appinfo.hxx"

const int TODO_APP_INFO_SIZE = 282;
     
class todoAppInfo_t : public appInfo_t
{
     int _dirty;
     int _sortByPriority;

     void *internalPack(void *);
     
   public:
     todoAppInfo_t(void *);

     int dirty(void) const { return _dirty; }
     int sortByPriority(void) const { return _sortByPriority; }

     void *pack(void);
};

class todoList_t;

class todo_t : public baseApp_t
{
     friend class todoList_t;
     
     struct tm *_due;		// Non-NULL if there is a due date
     int _priority;		// A priority in the range 1-5
     int _complete;		// true if the event was completed
     char *_description;	// The line that shows up in a ToDo list
     char *_note;		// Non-NULL if there is a note

     todo_t *_next;
     
     void *internalPack(unsigned char *);
     
   public:
     todo_t(void) : baseApp_t() { memset(this, '\0', sizeof(todo_t)); }
     todo_t(void *buf) : baseApp_t() { unpack(buf); }
     todo_t(void *buf, int attr, recordid_t id, int category)
	  : baseApp_t(attr, id, category)
	  {
	       unpack(buf);
	  }
     todo_t(const todo_t &);
     
     ~todo_t() {
	  if (_due) delete _due;
	  if (_description) delete _description;
	  if (_note) delete _note;
     }

     struct tm *due(void) const { return _due; }
     charConst_t description(void) const { return _description; }
     charConst_t note(void) const { return _note; }
     int priority(void) const { return _priority; }
     int complete(void) const { return _complete; }
     int completed(void) const { return _complete; }
     void unpack(void *);
     void *pack(int *);
     void *pack(void *, int *);
};

class todoList_t 
{
     todo_t *_head;

  public:
     todoList_t(void) : _head(NULL) { }
     ~todoList_t(void);

     todo_t *first() { return _head; }
     todo_t *next(todo_t *ptr) { return ptr->_next; }

     void merge(todo_t &);
     void merge(todoList_t &);
};
     
     
