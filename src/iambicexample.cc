
#include "pi-source.h"
#include <stdio.h>
#include <math.h>
#include <iostream.h>
#include <sys/types.h>
#include <stdlib.h>
#include "pi-file.h"
#include "pi-iambicExpense.h"

const char *const DEFAULT_FILE = "/afs/pdx/u/g/r/grosch/.xusrpilot/backup/ExpenseDB-IAMBIC.pdb";

static char *months[] = {
     "January", "February", "March", "April", "May", "June", "July",
     "August", "September", "October", "November", "December"
};

static char *commify(const double amount)
{
     static char buf[30], srcBuf[30];
     char *dst, *src;
     int i;
     
     (void) sprintf(srcBuf, "%.2f", amount);
     
     dst = &buf[sizeof(buf) - 1];
     src = &srcBuf[strlen(srcBuf) - 1];
     
     /* Copy in the cents and the decimal*/
     *dst-- = '\0';
     *dst-- = *src--;
     *dst-- = *src--;
     *dst-- = *src--;
     
     /* Now copy the dollars, adding in commas where necessary */
     for (i = 1; src >= srcBuf; src--, i++) {
          *dst-- = *src;
	  
          if (i % 3 == 0 && src != srcBuf)
               *dst-- = ',';
     }
     
     *dst = '$';
     
     return dst;
}

     
struct iambicExpenseTypes_t {
     char *name;
     double total;
     iambicExpenseTypes_t *next;
     iambicExpenseTypes_t() : name(NULL), total(0.0), next(NULL) {};
} *typesHead;

static void generateHeader(const int idx) 
{
     cout << "%!PS-Adobe-1.0" << endl
	  << "%%DocumentFonts: Helvetica" << endl
	  << "%%Title: XPilot Expenditures Report" << endl
	  << "%%Creator: XPilot 1.0" << endl
	  << "%%CreationDate: Fri Feb 28 13:48:43 1997" << endl
	  << "%%Pages: 1" << endl
	  << "%%EndComments" << endl
	  << "" << endl
	  << "/useColor true def      % Change this to false for black/white output" << endl
	  << "" << endl
	  << "/labelps 20 def		% The point size to use for pie chart labels" << endl
	  << "/titleps 50 def		% The point size to use the the title of the page" << endl
	  << "/titleYpos 720 def      % The y position to start the title at" << endl
	  << "" << endl
	  << "/colors [" << endl
	  << " [0 0 0] [0 1 1] [1 0 1] [1 1 0] [1 1 1] [0 0 .5] [0 .5 0] [0 .5 .5] [.5 0 0]" << endl
	  << " [.5 0 .5] [.5 .2 0] [1 .5 .5] [0 0 1] [0 1 0] [1 0 0]" << endl
	  << "] def" << endl
	  << "" << endl
	  << "/setColor {" << endl
	  << " /newLen colors length 1 sub def /curColor colors newLen get def" << endl
	  << " /colors colors aload pop pop newLen array astore def" << endl
	  << " curColor aload pop setrgbcolor % Leave this color on the stack" << endl
	  << "} def" << endl
	  << "" << endl
	  << "/printTitle { 0 0 moveto title show } def" << endl
	  << "" << endl
	  << "/DrawTitle {" << endl
	  << " gsave /Helvetica findfont titleps scalefont setfont " << endl
	  << " 306 title stringwidth pop 2 div sub titleYpos translate useColor " << endl
	  << " { .65 0 .90 setrgbcolor .50 -.05 0 { pop printTitle 1 -.5 " << endl
	  << " translate } for } { .95 -.05 0 { setgray printTitle 1 -.5 translate }" << endl
	  << " for } ifelse 1 setgray printTitle grestore" << endl
	  << "} def" << endl
	  << "" << endl
	  << "/DrawTotals" << endl
	  << "{" << endl
	  << " /x 36 def" << endl
	  << " /y titleYpos titleps sub 30 sub def" << endl
	  << "" << endl
	  << " newpath /Helvetica findfont labelps scalefont setfont" << endl
	  << "" << endl
	  << " /labelLength 0 def" << endl
	  << " /amountLength 0 def" << endl
	  << " dataArray {" << endl
	  << "	/elem exch def elem 0 get stringwidth pop /lenLabel exch def " << endl
	  << "        lenLabel labelLength gt { /labelLength lenLabel def } if" << endl
	  << "	elem 2 get stringwidth pop /lenAmount exch def" << endl
	  << "        lenAmount amountLength gt { /amountLength lenAmount def } if" << endl
	  << " } forall" << endl
	  << " /rightMargin labelLength 30 x lenAmount add add add def" << endl
	  << " rightMargin y labelps 2 div add moveto" << endl
	  << " /totHeight labelps 5 add dataArray length 1 sub mul def" << endl
	  << " /bottom y totHeight sub def" << endl
	  << " /totHeight totHeight 2 div def" << endl
	  << " /middle y totHeight sub def" << endl
	  << " rightMargin 90 add middle lineto" << endl
	  << " currentpoint /backtoY exch def /backtoX exch def" << endl
	  << " 10 labelps 2 div neg rmoveto " << endl
	  << " totalSpent show backtoX backtoY moveto" << endl
	  << " rightMargin bottom lineto" << endl
	  << " stroke " << endl
	  << " dataArray {" << endl
	  << "	/elem exch def elem aload pop /total exch def pop /label exch def" << endl
	  << "	x y moveto label show rightMargin total stringwidth pop sub y moveto " << endl
	  << "	total show /y y labelps sub 5 sub def" << endl
	  << " } forall" << endl
	  << "} def" << endl
	  << "" << endl
	  << "/DrawSlice {" << endl
	  << "  /grayshade exch def /endangle exch def /startangle exch def /thelabel exch def" << endl
	  << "  newpath 0 0 moveto 0 0 radius startangle endangle arc closepath" << endl
	  << "  1.415 setmiterlimit gsave useColor {setColor} {grayshade setgray} ifelse " << endl
	  << "  fill grestore stroke gsave startangle endangle add 2 div rotate radius 0 " << endl
	  << "  translate newpath 0 0 moveto labelps .8 mul 0 lineto stroke labelps 0 " << endl
	  << "  translate 0 0 transform grestore itransform /y exch def /x exch def x y " << endl
	  << "  moveto x 0 lt { thelabel stringwidth pop neg 0 rmoveto } if y 0 lt { 0 " << endl
	  << "  labelps neg rmoveto } if thelabel show" << endl
	  << "} def" << endl
	  << "" << endl
	  << "/findgray {" << endl
	  << " /i exch def /n exch def i 2 mod 0 eq { i 2 div n 2 div round add n div }" << endl
	  << " { i 1 add 2 div n div } ifelse" << endl
	  << "} def" << endl
	  << "" << endl
	  << "/DrawPieChart {" << endl
	  << " /radius 140 def gsave" << endl
	  << " 306 radius labelps .8 mul labelps 2 mul add add translate" << endl
	  << " /Helvetica findfont labelps scalefont setfont" << endl
	  << " /numslices dataArray length def /slicecnt 0 def /curangle 0 def" << endl
	  << " dataArray {" << endl
	  << "	/slicearray exch def slicearray aload pop /amount exch def" << endl
	  << "	/percent exch def /label exch def /perangle percent 360 mul def" << endl
	  << "	/slicecnt slicecnt 1 add def label curangle curangle perangle add" << endl
	  << "	numslices slicecnt findgray DrawSlice" << endl
	  << "	/curangle curangle perangle add def" << endl
	  << " } forall grestore" << endl
	  << "}def" << endl
	  << "" << endl
	  << "/doit {" << endl
	  << " /saveobj save def" << endl
	  << " /totalSpent exch def /dataArray exch def /title exch def " << endl
	  << " DrawTitle DrawTotals DrawPieChart" << endl
	  << " saveobj restore" << endl
	  << "} def" << endl
	  << "" << endl
	  << "%%EndProlog" << endl
	  << "" << endl
	  << "% ----------------------------------------------------------------------------" << endl
	  << "% This is the end of the procedures.  Data for each page follows" << endl
	  << "% ----------------------------------------------------------------------------" << endl
	  << "%%Page: 1 1" << endl << "(";

     cout << months[idx] << " Expenditures) [" << endl;
}

int main(int argc, char **argv) 
{
     pi_file *pf = pi_file_open((char *) DEFAULT_FILE);
     
     if (pf == NULL) {
          perror("pi_file_open");
          return 1;
     }

     int month;
     
     if (argc == 2)
	  month = atoi(argv[1]) - 1;
     else {
	  time_t curTime = time(NULL);
	  tm *curDate = localtime(&curTime);
	  month = curDate->tm_mon;
     }
     
     int nentries;
     pi_file_get_entries(pf, &nentries);
     
     unsigned char *buf;
     int attrs, cat;
     
     iambicExpense_t expense;
     const char *sptr;
     double exchangeRate, amount, total = 0.0;
     const tm *datePtr;
     iambicExpenseTypes_t *typePtr;
     
     typesHead = new iambicExpenseTypes_t;
     
     for (int entnum = 0; entnum < nentries; entnum++) {
          if (pi_file_read_record(pf, entnum, (void **) &buf, 0,
                                  &attrs, &cat, 0) < 0) {
               cout << "Error reading record number " << entnum << endl;
	       continue;
          }
	  
          /* Skip deleted records */
          if ((attrs & dlpRecAttrDeleted) || (attrs & dlpRecAttrArchived))
               continue;

	  expense.unpack(buf);
	  
	  // We only want to print records from this month
	  datePtr = expense.date();
	  if (datePtr->tm_mon != month)
	       continue;
	  
	  if ((sptr = expense.type()) == NULL)
	       sptr = "Unknown";
	  
	  amount = expense.amount();
	  
	  if ((exchangeRate = expense.exchangeRate()) != 1.0)
	       amount = floor(exchangeRate * amount * 100.0) / 100.0;
	  
	  for (typePtr = typesHead->next; typePtr != NULL; typePtr = typePtr->next)
	       if (!strcmp(typePtr->name, sptr)) {
		    typePtr->total += amount;
		    break;
	       }
	  
	  if (typePtr == NULL) {
	       typePtr = new iambicExpenseTypes_t;
	       typePtr->name = strdup(sptr);
	       typePtr->total = amount;
	       typePtr->next = typesHead->next;
	       typesHead->next = typePtr;
	  }
	  
	  total += amount;
     }

     generateHeader(month);
     
     double percent;
     
     for (typePtr = typesHead->next; typePtr != NULL; typePtr = typePtr->next) {
	  percent = typePtr->total / total;
	  cout << "  [(" << typePtr->name << ") " << percent << " ("
	       << commify(typePtr->total) << ")]" << endl;
     }

     cout << "] (" << commify(total) << ") doit" << endl << "showpage" << endl;
     cout << "%%Trailer" << endl;
     
     pi_file_close(pf);
     
     return 0;
}
