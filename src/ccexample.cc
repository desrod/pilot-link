#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
using namespace std;

#include <unistd.h>
#include <errno.h>

#include "pi-source.h"
#include "pi-file.h"
#include "pi-todo.h"
#include "pi-memo.h"
#include "pi-datebook.h"
#include "pi-address.h"

#define bool int
#define false 0
#define true 1

static char *days[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday"
};

static char *months[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December"
};

// All app info classes subclass from appInfo_t.  You could pass any of
// them to this function to print out the category names.  Mainly it's here
// to show the inheritance, as this has an appInfo_t as it's arguement, but
// can take any of the default app info classes.
void printCategoryNames(appInfo_t & ai)
{
	char *ptr;

	for (short int i = 0; i < 16; i++) {
		// This is sort of dangerous.  You are getting a pointer back to
		// the real data.  That means if you modify what ptr points to, you
		// are changing the value in the class itself.  Don't do that!
		// I can't find a way to keep this from happening.  Even if I do
		// something anal like return a const char *const all you
		// have to do is cast it to char * and then it's modifiable.
		ptr = ai.category(i);

		// The first character will be non-null for an existing category
		if (*ptr)
			cout << "Category " << (i +
						1) << " is " << ptr <<
			    endl;
	}
}

void memos(pi_file * pf)
{
	void *app_info;
	size_t app_info_size;

	if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
		cerr << "Unable to get app info" << endl;
		return;
	}
	// Create mai as an unpacked structure with the memo app info
	memoAppInfo_t mai(app_info);

	// packed is now a pointer to an area of memory that is MEMO_APP_INFO_SIZE
	// bytes long.  You are responsible for release of this memory via delete
	unsigned char* packed = (unsigned char*)mai.pack();
	delete[]packed;

	int nentries;

	pi_file_get_entries(pf, &nentries);

	unsigned char *buf, packedBuf[0xffff];
	int size, attrs, cat;
	recordid_t uid;

	memo_t memo;

	for (int entnum = 0; entnum < nentries; entnum++) {
		if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
					&attrs, &cat, &uid) < 0) {
			cout << "Error reading record number " << entnum <<
			    endl;
			return;
		}

		/* Skip deleted records */
		if ((attrs & dlpRecAttrDeleted)
		    || (attrs & dlpRecAttrArchived))
			continue;

		memo.unpack(buf);

		cout << "Memo number " << (entnum + 1) << endl;
		cout << memo.text() << endl << endl;

		// Option 1 for getting a packed memo. Just give an int to be
		// filled in with the size of the memo. You must free the space
		// returned via delete
		unsigned char* packed = (unsigned char*)memo.pack(&size);
		delete[] packed;

		// Option 2 for getting a packed memo.  Give a buffer, and an int
		// telling how big it is.  If the buffer is too small to hold the
		// packed data, NULL is returned.  Otherwise, the buffer is filled
		// in with the packed data, a pointer to it is returned, and the
		// integer passed in is reset to be the size of the packed data
		if (memo.pack(packedBuf, &size) == NULL)
			cerr << "Record number " << (entnum +
						     1) << " too big for "
			    << "the buffer you passed in." << endl;
	}
}


void todos(pi_file * pf)
{
	void *app_info;
	int app_info_size;

	if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
		cerr << "Unable to get app info" << endl;
		return;
	}

	todoAppInfo_t tai(app_info);

	// packed is now a pointer to an area of memory that is TODO_APP_INFO_SIZE
	// bytes long.  You are responsible for release this memory via delete
	unsigned char* packed = (unsigned char*)tai.pack();
	delete[] packed;

	int nentries;

	pi_file_get_entries(pf, &nentries);

	unsigned char *buf, packedBuf[0xffff];
	int size, attrs, cat;
	recordid_t uid;
	tm *due;
	todo_t todo;

	for (int entnum = 0; entnum < nentries; entnum++) {
		if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
					&attrs, &cat, &uid) < 0) {
			cout << "Error reading record number " << entnum <<
			    endl;
			return;
		}

		/* Skip deleted records */
		if ((attrs & dlpRecAttrDeleted)
		    || (attrs & dlpRecAttrArchived))
			continue;

		todo.unpack(buf);

		cout << "Category: " << tai.category(cat) << endl;
		if (todo.description())
			cout << "Description: " << todo.
			    description() << endl;
		cout << "Priority: " << todo.priority() << endl;
		cout << "Completed: " << (todo.
					  complete()? "Yes" : "No") <<
		    endl;
		if ((due = todo.due()))
			cout << "Due: " << asctime(due);
		else
			cout << "Due: No Date" << endl;
		if (todo.note())
			cout << "Note: " << todo.note() << endl;
		cout << endl;

		// Just like the memo app, you can pack it like this...
		unsigned char* packed = (unsigned char*)todo.pack(&size);
		delete[] packed;

		// ... or like this
		size = sizeof(packedBuf);
		if (todo.pack(packedBuf, &size) == NULL)
			cerr << "Record number " << (entnum +
						     1) << " too big for "
			    << "the buffer you passed in." << endl;
	}
}

// Only works for up to i == 31.  Beyond that, results are undefined
char *freqToStr(const int i)
{
	static char buf[7];

	if (i == 1)
		buf[0] = '\0';
	else if (i == 3 || i == 23)
		(void) sprintf(buf, "%drd ", i);
	else if ((i > 3 && i < 21) || (i > 23 && i < 31))
		(void) sprintf(buf, "%dth ", i);
	else if (i == 21 || i == 31)
		(void) sprintf(buf, "%dst ", i);
	else if (i == 2 || i == 22)
		(void) sprintf(buf, "%dnd ", i);

	return buf;
}


void prettyPrintRepeat(appointment_t * appt)
{
	tm *timePtr;

	if ((timePtr = appt->repeatEnd()))
		cout << "This event repeats until " << asctime(timePtr);
	else
		cout << "This event repeats forever" << endl;

	int freq = appt->repeatFreq();
	int on = appt->repeatOn();

	bool found = false;

	switch (appt->repeatType()) {
	  case appointment_t::daily:
		  cout << "It repeats every " << freqToStr(freq) << "day";
		  break;
	  case appointment_t::weekly:
	  {
		  cout << "It repeats every " << freqToStr(freq) <<
		      "week on ";

		  for (int i = 0; i < 7; i++) {
			  if (on & (1 << i)) {
				  if (found)
					  cout << " and ";
				  else
					  found = true;
				  cout << days[i];
			  }
		  }
		  cout << endl;
		  break;
	  }
	  case appointment_t::monthlyByDay:
		  cout << "It repeats on the ";

		  switch (on / 7) {
		    case 0:
			    cout << "first ";
			    break;
		    case 1:
			    cout << "second ";
			    break;
		    case 2:
			    cout << "third ";
			    break;
		    case 3:
			    cout << "fourth ";
			    break;
		    default:
			    cout << "last ";
		  }

		  cout << days[on % 7] << " of every month";
		  break;
	  case appointment_t::monthlyByDate:
		  cout << "It repeats every " << freqToStr(freq);
		  timePtr = appt->beginTime();
		  cout << "month on the " << freqToStr(timePtr->
						       tm_mday) << endl;
		  cout << endl;
		  break;
	  case appointment_t::yearly:
		  cout << "It repeats every " << freqToStr(freq) <<
		      "year on ";
		  timePtr = appt->beginTime();
		  cout << months[timePtr->tm_mon] << " " << timePtr->
		      tm_mday;
		  cout << endl;
		  break;
	  default:
		  cerr << "Internal error" << endl;
	}
}

void datebook(pi_file * pf)
{
	void *app_info;
	int app_info_size;

	if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
		cerr << "Unable to get app info" << endl;
		return;
	}

	appointmentAppInfo_t aai(app_info);

	// packed is now a pointer to an area of memory that is
	// APPOINTMENT_APP_INFO_SIZE bytes long.  You are responsible for
	// release this memory via delete
	unsigned char* packed = (unsigned char*)aai.pack();
	delete[] packed;

	int nentries;

	pi_file_get_entries(pf, &nentries);

	unsigned char *buf;
	int size, attrs, cat;
	recordid_t uid;
	tm *timePtr;
	appointment_t appt;

	for (int entnum = 0; entnum < nentries; entnum++) {
		if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
					&attrs, &cat, &uid) < 0) {
			cout << "Error reading record number " << entnum <<
			    endl;
			return;
		}

		/* Skip deleted records */
		if ((attrs & dlpRecAttrDeleted)
		    || (attrs & dlpRecAttrArchived))
			continue;

		appt.unpack(buf);

		if (appt.untimed() == false) {
			cout << "Begin Time:  " << asctime(appt.
							   beginTime());
			cout << "End Time:    " << asctime(appt.endTime());
		} else
			cout << "Untimed event" << endl;

		if (appt.hasAlarm()) {
			cout << "The alarm is set to go off " << appt.
			    advance() << " ";

			switch (appt.advanceUnits()) {
			  case appointment_t::minutes:
				  cout << "minutes";
				  break;
			  case appointment_t::hours:
				  cout << "hours";
				  break;
			  case appointment_t::days:
				  cout << "days";
			  default:
				  cout << "(internal error)";
			}

			cout << " before the event" << endl;
		} else
			cout << "There is not an alarm set for this event"
			    << endl;

		if (appt.repeatType() != appointment_t::none)
			prettyPrintRepeat(&appt);
		else
			cout << "Event does not repeat" << endl;

		if ((timePtr = appt.exceptions())) {
			size = appt.numExceptions();
			cout << "I seem to have " << size << " exceptions:"
			    << endl;
			for (int i = 0; i < size; i++)
				cout << "\t" << asctime(&timePtr[i]);
		}

		cout << "Description: " << appt.description() << endl;

		if (appt.note())
			cout << "Note: " << appt.note() << endl;

		cout << endl;
	}
}


void addresses(pi_file * pf)
{

	void *app_info;
	int app_info_size;

	if (pi_file_get_app_info(pf, &app_info, &app_info_size) < 0) {
		cerr << "Unable to get app info" << endl;
		return;
	}

	addressAppInfo_t aai(app_info);

	printCategoryNames(aai);

	// packed is now a pointer to an area of memory that is
	// ADDRESS_APP_INFO_SIZE bytes long.  You are responsible for release this
	// memory via delete
	unsigned char* packed = (unsigned char*)aai.pack();
	delete[] packed;

	int nentries;

	pi_file_get_entries(pf, &nentries);

	unsigned char *buf, packedBuf[0xffff];
	int size, attrs, cat;
	recordid_t uid;
	address_t address;
	char *phonePtr;

	for (int entnum = 0; entnum < nentries; entnum++) {
		if (pi_file_read_record(pf, entnum, (void **) &buf, &size,
					&attrs, &cat, &uid) < 0) {
			cout << "Error reading record number " << entnum <<
			    endl;
			return;
		}

		/* Skip deleted records */
		if ((attrs & dlpRecAttrDeleted)
		    || (attrs & dlpRecAttrArchived))
			continue;

		address.unpack(buf);

		cout << "Category: " << aai.category(cat) << endl;
		phonePtr = address.entry(address_t::lastName);
		if (phonePtr)
			cout << "Last Name: " << phonePtr << endl;
		for (cat = address_t::phone1; cat <= address_t::phone5;
		     cat++)
			if ((phonePtr =
			     address.entry((address_t::labelTypes_t) cat)))
				cout << "Phone:  " << phonePtr << endl;

		// Just like the memo app, you can pack it like this...
		unsigned char* packed = (unsigned char*)address.pack(&size);
		delete[] packed;

		// ... or like this
		size = sizeof(packedBuf);
		if (address.pack(packedBuf, &size) == NULL)
			cerr << "Record number " << (entnum +
						     1) << " too big for "
			    << "the buffer you passed in." << endl;
	}
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		cerr << "Usage: " << *argv << " [.pdb file]" << endl;
		return 1;
	}

	pi_file *pf;

	if ((pf = pi_file_open(*(argv + 1))) == NULL) {
		perror("pi_file_open");
		return 1;
	}

	char *slash = strrchr(*(argv + 1), '/');

	if (slash)
		slash++;
	else
		slash = *(argv + 1);

	if (!strcmp(slash, "ToDoDB.pdb"))
		todos(pf);
	else if (!strcmp(slash, "DatebookDB.pdb"))
		datebook(pf);
	else if (!strcmp(slash, "AddressDB.pdb"))
		addresses(pf);
	else if (!strcmp(slash, "MemoDB.pdb"))
		memos(pf);
	else
		cerr << "Unknown database: " << slash << endl;

	pi_file_close(pf);

	return 0;
}
