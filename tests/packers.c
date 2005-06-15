/* packers.c:  Test packing functions
 *
 * Copyright (c) 1997, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Public License V2.
 * See the file COPYING for details.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-memo.h"
#include "pi-address.h"
#include "pi-datebook.h"
#include "pi-todo.h"
#include "pi-dlp.h"
#include "pi-expense.h"
#include "pi-mail.h"

unsigned char seed;
char *target;
int targetlen;


void reset_block(char *buffer, int len)
{
   unsigned int i;

   for (i = 0; i < len; i++)
      buffer[i] = (i + seed) & 0xff;
}

int check_block(int test, const char *buffer, int len, int start,
		int count, const char *name)
{
   unsigned int i;
   int fore = 0, aft = 0;

   for (i = 0; i < start; i++)
      if (buffer[i] != (char) ((i + seed) & 0xff)) {
	 fore = start - i;
	 break;
      }
   for (i = start + count; i < len; i++)
      if (buffer[i] != (char) ((i + seed) & 0xff)) {
	 aft = i - start;
	 break;
      }
   if (fore || aft) {
      printf("%d: %s scribbled ", test, name);
      if (fore)
	 printf("%d byte(s) before", fore);
      if (fore && aft)
	 printf(", and ");
      if (aft)
	 printf("%d byte(s) after", aft);
      printf(" the allocated buffer.\n");
      return 1;
   }
   return 0;
}

char MemoAppBlock[17 * 16 + 10] = "\
\x00\x00\x55\x6e\x66\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x42\x75\x73\x69\x6e\x65\x73\x73\x00\x00\x00\x00\x00\x00\
\x00\x00\x50\x65\x72\x73\x6f\x6e\x61\x6c\x00\x00\x00\x00\x00\x00\
\x00\x00\x54\x65\x63\x68\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x54\x65\x73\x74\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x42\x6f\x6f\x6b\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x53\x74\x6f\x72\x79\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x01\x02\x10\x11\x12\x13\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x13\x00\x00\x00\x00\x00\x01\x00";

char MemoRecord[3 * 16 + 7] = "\
\x61\x61\x4d\x61\x6b\x65\x66\x69\x6c\x65\x0a\x52\x45\x41\x44\x4d\
\x45\x0a\x6c\x69\x63\x65\x6e\x73\x65\x2e\x74\x65\x72\x6d\x73\x0a\
\x70\x69\x6c\x6f\x74\x6c\x69\x6e\x6b\x2e\x63\x0a\x74\x65\x73\x74\
\x2e\x74\x63\x6c\x2a\x0a\x00";

int test_memo()
{
   struct MemoAppInfo mai;
   struct Memo m;
   pi_buffer_t *RecordBuffer;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l = unpack_MemoAppInfo(&mai, MemoAppBlock, sizeof(MemoAppBlock) + 10);
   if (l != sizeof(MemoAppBlock)) {
      errors++;
      printf
	  ("1: unpack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MemoAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l = unpack_MemoAppInfo(&mai, MemoAppBlock, sizeof(MemoAppBlock) + 1);
   if (l != sizeof(MemoAppBlock)) {
      errors++;
      printf
	  ("2: unpack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MemoAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l = unpack_MemoAppInfo(&mai, MemoAppBlock, sizeof(MemoAppBlock) - 10);
   if (l != 0) {
      errors++;
      printf
	  ("3: unpack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return failure if the block is too small to contain data */
   /* Note: -1 isn't used, because four bytes _can_ be subtracted, to account for
      the new data in OS 2.0 */
   l = unpack_MemoAppInfo(&mai, MemoAppBlock, sizeof(MemoAppBlock) - 5);
   if (l != 0) {
      errors++;
      printf
	  ("4: unpack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return count of bytes used */
   l = unpack_MemoAppInfo(&mai, MemoAppBlock, sizeof(MemoAppBlock));
   if (l != sizeof(MemoAppBlock)) {
      errors++;
      printf
	  ("5: unpack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MemoAppBlock));
   }

   if (
       (mai.sortByAlpha == 0) ||
       strcmp(mai.category.name[0], "Unfiled") ||
       strcmp(mai.category.name[1], "Business") ||
       strcmp(mai.category.name[2], "Personal") ||
       strcmp(mai.category.name[3], "Tech") ||
       strcmp(mai.category.name[4], "Test") ||
       strcmp(mai.category.name[5], "Books") ||
       strcmp(mai.category.name[6], "Story") ||
       strcmp(mai.category.name[7], "") ||
       strcmp(mai.category.name[8], "") ||
       strcmp(mai.category.name[9], "") ||
       strcmp(mai.category.name[10], "") ||
       strcmp(mai.category.name[11], "") ||
       strcmp(mai.category.name[12], "") ||
       strcmp(mai.category.name[13], "") ||
       strcmp(mai.category.name[14], "") ||
       strcmp(mai.category.name[15], "") ||
       mai.category.renamed[0] ||
       mai.category.renamed[1] ||
       mai.category.renamed[2] ||
       mai.category.renamed[3] ||
       mai.category.renamed[4] ||
       mai.category.renamed[5] ||
       mai.category.renamed[6] ||
       mai.category.renamed[7] ||
       mai.category.renamed[8] ||
       mai.category.renamed[9] ||
       mai.category.renamed[10] ||
       mai.category.renamed[11] ||
       mai.category.renamed[12] ||
       mai.category.renamed[13] ||
       mai.category.renamed[14] ||
       mai.category.renamed[15] ||
       (mai.category.ID[0] != 0) ||
       (mai.category.ID[1] != 1) ||
       (mai.category.ID[2] != 2) ||
       (mai.category.ID[3] != 16) ||
       (mai.category.ID[4] != 17) ||
       (mai.category.ID[5] != 18) ||
       (mai.category.ID[6] != 19) ||
       (mai.category.ID[7] != 0) ||
       (mai.category.ID[8] != 0) ||
       (mai.category.ID[9] != 0) ||
       (mai.category.ID[10] != 0) ||
       (mai.category.ID[11] != 0) ||
       (mai.category.ID[12] != 0) ||
       (mai.category.ID[13] != 0) ||
       (mai.category.ID[14] != 0) ||
       (mai.category.ID[15] != 0) ||
       (mai.category.lastUniqueID != 19) || 0) {
      errors++;
      printf("6: unpack_MemoAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_MemoAppInfo(&mai, 0, 0);
   if (l != sizeof(MemoAppBlock)) {
      errors++;
      printf
	  ("7: pack_MemoAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(MemoAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_MemoAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf("8: pack_MemoAppInfo packed into too small buffer (got %d)\n",
	     l);
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, 1, "pack_MemoAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_MemoAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(MemoAppBlock)) {
      errors++;
      printf
	  ("10: pack_MemoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MemoAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(11, target, 8192, 128, l, "pack_MemoAppInfo"))
      errors++;

   if (memcmp(target + 128, MemoAppBlock, sizeof(MemoAppBlock))) {
      errors++;
      printf
	  ("12: pack_MemoAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MemoAppBlock, sizeof(MemoAppBlock));
   }

   RecordBuffer = pi_buffer_new(sizeof(MemoRecord));
   memcpy(RecordBuffer->data, MemoRecord, RecordBuffer->allocated);
   RecordBuffer->used=sizeof(MemoRecord);
   unpack_Memo(&m, RecordBuffer, memo_v1);
   pi_buffer_free(RecordBuffer);

   if (
       (m.text == 0) ||
       strcmp(m.text,
	      "aaMakefile\x0aREADME\x0alicense.terms\x0apilotlink.c\x0atest.tcl*\x0a"))
   {
      errors++;
      printf("13: unpack_Memo generated incorrect information\n");
   }
   
   RecordBuffer = pi_buffer_new(0);
   if (pack_Memo(&m, RecordBuffer, memo_v1) != 0) {
      errors++;
     printf("14: pack_Memo returned failure\n");
   }

   if (RecordBuffer->used != sizeof(MemoRecord)) {
      errors++;
      printf
	  ("15: pack_MemoRecord returned incorrect allocation length (got %d, expected %d)\n",
	   RecordBuffer->used, sizeof(MemoRecord));
   }

   if (memcmp(RecordBuffer->data, MemoRecord, sizeof(MemoRecord))) {
      errors++;
      printf("16: pack_Memo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MemoRecord, sizeof(MemoRecord));
   }

   pi_buffer_free(RecordBuffer);

   printf("Memo packers test completed with %d error(s).\n", errors);

   return errors;
}

char AddressAppBlock[39 * 16 + 14] = "\
\x00\x10\x55\x6e\x66\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x42\x75\x73\x69\x6e\x65\x73\x73\x00\x00\x00\x00\x00\x00\
\x00\x00\x50\x65\x72\x73\x6f\x6e\x61\x6c\x00\x00\x00\x00\x00\x00\
\x00\x00\x51\x75\x69\x63\x6b\x4c\x69\x73\x74\x00\x00\x00\x00\x00\
\x00\x00\x46\x6f\x6f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x01\x02\x03\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x11\x00\x00\x00\x00\x00\x0e\x00\x4c\x61\x73\x74\x20\x6e\
\x61\x6d\x65\x00\x00\x00\x00\x00\x00\x00\x46\x69\x72\x73\x74\x20\
\x6e\x61\x6d\x65\x00\x00\x00\x00\x00\x00\x43\x6f\x6d\x70\x61\x6e\
\x79\x00\x00\x00\x00\x00\x00\x00\x00\x00\x57\x6f\x72\x6b\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x48\x6f\x6d\x65\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x46\x61\x78\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4f\x74\x68\x65\x72\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x45\x2d\x6d\x61\x69\x6c\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x41\x64\x64\x72\x65\x73\
\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00\x43\x69\x74\x79\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x53\x74\x61\x74\x65\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x5a\x69\x70\x20\x43\x6f\
\x64\x65\x00\x00\x00\x00\x00\x00\x00\x00\x43\x6f\x75\x6e\x74\x72\
\x79\x00\x00\x00\x00\x00\x00\x00\x00\x00\x54\x69\x74\x6c\x65\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x43\x75\x73\x74\x6f\x6d\
\x20\x31\x00\x00\x00\x00\x00\x00\x00\x00\x43\x75\x73\x74\x6f\x6d\
\x20\x32\x00\x00\x00\x00\x00\x00\x00\x00\x43\x75\x73\x74\x6f\x6d\
\x20\x33\x00\x00\x00\x00\x00\x00\x00\x00\x43\x75\x73\x74\x6f\x6d\
\x20\x34\x00\x00\x00\x00\x00\x00\x00\x00\x4e\x6f\x74\x65\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4d\x61\x69\x6e\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x50\x61\x67\x65\x72\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x4d\x6f\x62\x69\x6c\x65\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x17\x00\x00\x00";

char AddressRecord[2 * 16 + 12] = "\
\x00\x14\x32\x10\x00\x04\x41\x03\x00\x53\x68\x61\x77\x00\x42\x65\
\x72\x6e\x61\x72\x64\x00\x4e\x6f\x6e\x65\x20\x6b\x6e\x6f\x77\x6e\
\x00\x43\x31\x00\x41\x20\x6e\x6f\x74\x65\x2e\x00";

int test_address()
{
   struct AddressAppInfo mai;
   struct Address m;
   pi_buffer_t *RecordBuffer;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l =
       unpack_AddressAppInfo(&mai, AddressAppBlock,
			     sizeof(AddressAppBlock) + 10);
   if (l != sizeof(AddressAppBlock)) {
      errors++;
      printf
	  ("1: unpack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AddressAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_AddressAppInfo(&mai, AddressAppBlock,
			     sizeof(AddressAppBlock) + 1);
   if (l != sizeof(AddressAppBlock)) {
      errors++;
      printf
	  ("2: unpack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AddressAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l =
       unpack_AddressAppInfo(&mai, AddressAppBlock,
			     sizeof(AddressAppBlock) - 10);
   if (l != 0) {
      errors++;
      printf
	  ("3: unpack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l =
       unpack_AddressAppInfo(&mai, AddressAppBlock,
			     sizeof(AddressAppBlock) - 1);
   if (l != 0) {
      errors++;
      printf
	  ("4: unpack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_AddressAppInfo(&mai, AddressAppBlock,
			     sizeof(AddressAppBlock));
   if (l != sizeof(AddressAppBlock)) {
      errors++;
      printf
	  ("5: unpack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AddressAppBlock));
   }

   if (strcmp(mai.category.name[0], "Unfiled") ||
       strcmp(mai.category.name[1], "Business") ||
       strcmp(mai.category.name[2], "Personal") ||
       strcmp(mai.category.name[3], "QuickList") ||
       strcmp(mai.category.name[4], "Foo") ||
       strcmp(mai.category.name[5], "") ||
       strcmp(mai.category.name[6], "") ||
       strcmp(mai.category.name[7], "") ||
       strcmp(mai.category.name[8], "") ||
       strcmp(mai.category.name[9], "") ||
       strcmp(mai.category.name[10], "") ||
       strcmp(mai.category.name[11], "") ||
       strcmp(mai.category.name[12], "") ||
       strcmp(mai.category.name[13], "") ||
       strcmp(mai.category.name[14], "") ||
       strcmp(mai.category.name[15], "") ||
       mai.category.renamed[0] ||
       mai.category.renamed[1] ||
       mai.category.renamed[2] ||
       mai.category.renamed[3] ||
       (!mai.category.renamed[4]) ||
       mai.category.renamed[5] ||
       mai.category.renamed[6] ||
       mai.category.renamed[7] ||
       mai.category.renamed[8] ||
       mai.category.renamed[9] ||
       mai.category.renamed[10] ||
       mai.category.renamed[11] ||
       mai.category.renamed[12] ||
       mai.category.renamed[13] ||
       mai.category.renamed[14] ||
       mai.category.renamed[15] ||
       (mai.category.ID[0] != 0) ||
       (mai.category.ID[1] != 1) ||
       (mai.category.ID[2] != 2) ||
       (mai.category.ID[3] != 3) ||
       (mai.category.ID[4] != 17) ||
       (mai.category.ID[5] != 0) ||
       (mai.category.ID[6] != 0) ||
       (mai.category.ID[7] != 0) ||
       (mai.category.ID[8] != 0) ||
       (mai.category.ID[9] != 0) ||
       (mai.category.ID[10] != 0) ||
       (mai.category.ID[11] != 0) ||
       (mai.category.ID[12] != 0) ||
       (mai.category.ID[13] != 0) ||
       (mai.category.ID[14] != 0) ||
       (mai.category.ID[15] != 0) ||
       (mai.category.lastUniqueID != 17) || 0) {
      errors++;
      printf("6: unpack_AddressAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_AddressAppInfo(&mai, 0, 0);
   if (l != sizeof(AddressAppBlock)) {
      errors++;
      printf
	  ("7: pack_AddressAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(AddressAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_AddressAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf
	  ("8: pack_AddressAppInfo packed into too small buffer (got %d)\n",
	   l);
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, 1, "pack_AddressAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_AddressAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(AddressAppBlock)) {
      errors++;
      printf
	  ("10: pack_AddressAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AddressAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(11, target, 8192, 128, l, "pack_AddressAppInfo"))
      errors++;

   if (memcmp(target + 128, AddressAppBlock, sizeof(AddressAppBlock))) {
      errors++;
      printf
	  ("12: pack_AddressAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(AddressAppBlock, sizeof(AddressAppBlock));
   }


   /* Unpacker should return count of bytes used */
   RecordBuffer = pi_buffer_new(sizeof(AddressRecord));
   memcpy(RecordBuffer->data, AddressRecord, RecordBuffer->allocated);
   RecordBuffer->used=sizeof(AddressRecord);
   unpack_Address(&m, RecordBuffer, address_v1);
   pi_buffer_free(RecordBuffer);

   if (
       (m.phoneLabel[0] != 0) ||
       (m.phoneLabel[1] != 1) ||
       (m.phoneLabel[2] != 2) ||
       (m.phoneLabel[3] != 3) ||
       (m.phoneLabel[4] != 4) ||
       strcmp(m.entry[0], "Shaw") ||
       strcmp(m.entry[1], "Bernard") ||
       m.entry[2] ||
       m.entry[3] ||
       strcmp(m.entry[8], "None known") ||
       strcmp(m.entry[14], "C1") || (m.showPhone != 1) || 0) {
      errors++;
      printf("13: unpack_Address generated incorrect information\n");
   }

   RecordBuffer = pi_buffer_new(0);
   if (pack_Address(&m, RecordBuffer, address_v1) != 0) {
      errors++;
     printf("14: pack_Address returned failure\n");
   }

   if (RecordBuffer->used != sizeof(AddressRecord)) {
      errors++;
      printf
	  ("15: pack_Address returned incorrect length (got %d, expected %d)\n",
	   RecordBuffer->used, sizeof(AddressRecord));
   }

   if (memcmp(RecordBuffer->data, AddressRecord, sizeof(AddressRecord))) {
      errors++;
      printf("16: pack_Address generated incorrect information. Got:\n");
      pi_dumpdata(RecordBuffer->data, l);
      printf(" expected:\n");
      pi_dumpdata(AddressRecord, sizeof(AddressRecord));
   }
   
   pi_buffer_free(RecordBuffer);

   printf("Address packers test completed with %d error(s).\n", errors);

   return errors;
}

char AppointmentAppBlock[17 * 16 + 8] = "\
\x00\x00\x55\x6e\x66\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00";

/* Note: Bytes seven and seventeen ares undefined by the Pilot code,
   and thus have a floating value. This sample record has
   been altered to make theses bytes zero, to match what our
   packing code generates.  */
char AppointmentRecord[2 * 16 + 3] = "\
\x09\x00\x0d\x28\xbb\x02\x7c\x00\x1d\x02\x02\x00\xbd\x24\x02\x55\
\x00\x00\x00\x01\xbb\x0c\x47\x65\x6f\x72\x67\x65\x00\x4e\x6f\x74\
\x65\x21\x00";


int test_appointment()
{
   struct AppointmentAppInfo mai;
   struct Appointment m;
   pi_buffer_t *RecordBuffer;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l =
       unpack_AppointmentAppInfo(&mai, AppointmentAppBlock,
				 sizeof(AppointmentAppBlock) + 10);
   if (l != sizeof(AppointmentAppBlock)) {
      errors++;
      printf
	  ("1: unpack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AppointmentAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_AppointmentAppInfo(&mai, AppointmentAppBlock,
				 sizeof(AppointmentAppBlock) + 1);
   if (l != sizeof(AppointmentAppBlock)) {
      errors++;
      printf
	  ("2: unpack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AppointmentAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l =
       unpack_AppointmentAppInfo(&mai, AppointmentAppBlock,
				 sizeof(AppointmentAppBlock) - 10);
   if (l != 0) {
      errors++;
      printf
	  ("3: unpack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l =
       unpack_AppointmentAppInfo(&mai, AppointmentAppBlock,
				 sizeof(AppointmentAppBlock) - 1);
   if (l != 0) {
      errors++;
      printf
	  ("4: unpack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_AppointmentAppInfo(&mai, AppointmentAppBlock,
				 sizeof(AppointmentAppBlock));
   if (l != sizeof(AppointmentAppBlock)) {
      errors++;
      printf
	  ("5: unpack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AppointmentAppBlock));
   }

   if (
/*     strcmp(mai.category.name[0],"Unfiled") ||
     strcmp(mai.category.name[1],"Business") ||
     strcmp(mai.category.name[2],"Personal") ||
     strcmp(mai.category.name[3],"QuickList") ||
     strcmp(mai.category.name[4],"Foo") ||
     strcmp(mai.category.name[5],"") ||
     strcmp(mai.category.name[6],"") ||
     strcmp(mai.category.name[7],"") ||
     strcmp(mai.category.name[8],"") ||
     strcmp(mai.category.name[9],"") ||
     strcmp(mai.category.name[10],"") ||
     strcmp(mai.category.name[11],"") ||
     strcmp(mai.category.name[12],"") ||
     strcmp(mai.category.name[13],"") ||
     strcmp(mai.category.name[14],"") ||
     strcmp(mai.category.name[15],"") ||
     mai.category.renamed[0] ||
     mai.category.renamed[1] ||
     mai.category.renamed[2] ||
     mai.category.renamed[3] ||
     (!mai.category.renamed[4]) ||
     mai.category.renamed[5] ||
     mai.category.renamed[6] ||
     mai.category.renamed[7] ||
     mai.category.renamed[8] ||
     mai.category.renamed[9] ||
     mai.category.renamed[10] ||
     mai.category.renamed[11] ||
     mai.category.renamed[12] ||
     mai.category.renamed[13] ||
     mai.category.renamed[14] ||
     mai.category.renamed[15] ||
     (mai.category.ID[0] != 0) ||
     (mai.category.ID[1] != 1) ||
     (mai.category.ID[2] != 2) ||
     (mai.category.ID[3] != 3) ||
     (mai.category.ID[4] != 17) ||
     (mai.category.ID[5] != 0) ||
     (mai.category.ID[6] != 0) ||
     (mai.category.ID[7] != 0) ||
     (mai.category.ID[8] != 0) ||
     (mai.category.ID[9] != 0) ||
     (mai.category.ID[10] != 0) ||
     (mai.category.ID[11] != 0) ||
     (mai.category.ID[12] != 0) ||
     (mai.category.ID[13] != 0) ||
     (mai.category.ID[14] != 0) ||
     (mai.category.ID[15] != 0) ||
     (mai.category.lastUniqueID != 17)  ||*/
	 0) {
      errors++;
      printf
	  ("6: unpack_AppointmentAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_AppointmentAppInfo(&mai, 0, 0);
   if (l != sizeof(AppointmentAppBlock)) {
      errors++;
      printf
	  ("7: pack_AppointmentAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(AppointmentAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_AppointmentAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf
	  ("8: pack_AppointmentAppInfo packed into too small buffer (got %d)\n",
	   l);
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, 1, "pack_AppointmentAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_AppointmentAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(AppointmentAppBlock)) {
      errors++;
      printf
	  ("10: pack_AppointmentAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(AppointmentAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(11, target, 8192, 128, l, "pack_AppointmentAppInfo"))
      errors++;

   if (memcmp
       (target + 128, AppointmentAppBlock, sizeof(AppointmentAppBlock))) {
      errors++;
      printf
	  ("12: pack_AppointmentAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(AppointmentAppBlock, sizeof(AppointmentAppBlock));
   }


   RecordBuffer = pi_buffer_new(sizeof(AppointmentRecord));
   memcpy(RecordBuffer->data, AppointmentRecord, RecordBuffer->allocated);
   RecordBuffer->used=sizeof(AppointmentRecord);
   unpack_Appointment(&m, RecordBuffer, datebook_v1);
   pi_buffer_free(RecordBuffer);

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("13: unpack_Appointment generated incorrect information\n");
   }

   RecordBuffer = pi_buffer_new(0);
   if (pack_Appointment(&m, RecordBuffer, datebook_v1) != 0) {
      errors++;
     printf("14: pack_Appointment returned failure\n");
   }

   if (RecordBuffer->used != sizeof(AppointmentRecord)) {
      errors++;
      printf
	  ("15: pack_Appointment returned incorrect length (got %d, expected %d)\n",
	   RecordBuffer->used, sizeof(AppointmentRecord));
   }

   if (memcmp(RecordBuffer->data, AppointmentRecord, sizeof(AppointmentRecord))) {
      errors++;
      printf
	  ("16: pack_Appointment generated incorrect information. Got:\n");
      pi_dumpdata(RecordBuffer->data, l);
      printf(" expected:\n");
      pi_dumpdata(AppointmentRecord, sizeof(AppointmentRecord));
   }

   printf("Appointment packers test completed with %d error(s).\n",
	  errors);

   return errors;
}

char ToDoAppBlock[17 * 16 + 10] = "\
\x00\x08\x55\x6e\x66\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x42\x75\x73\x69\x6e\x65\x73\x73\x00\x00\x00\x00\x00\x00\
\x00\x00\x50\x65\x72\x73\x6f\x6e\x61\x6c\x00\x00\x00\x00\x00\x00\
\x00\x00\x46\x6f\x6f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x01\x02\x11\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x11\x00\x00\x00\xff\xff\x01\x00";

char ToDoRecord[1 * 16 + 1] = "\
\xbb\x09\x05\x54\x6f\x64\x6f\x33\x00\x41\x20\x6e\x6f\x74\x65\x2e\
\x00";

int test_todo()
{
   struct ToDoAppInfo mai;
   struct ToDo m;
   pi_buffer_t *RecordBuffer;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l = unpack_ToDoAppInfo(&mai, ToDoAppBlock, sizeof(ToDoAppBlock) + 10);
   if (l != sizeof(ToDoAppBlock)) {
      errors++;
      printf
	  ("1: unpack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ToDoAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l = unpack_ToDoAppInfo(&mai, ToDoAppBlock, sizeof(ToDoAppBlock) + 1);
   if (l != sizeof(ToDoAppBlock)) {
      errors++;
      printf
	  ("2: unpack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ToDoAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l = unpack_ToDoAppInfo(&mai, ToDoAppBlock, sizeof(ToDoAppBlock) - 10);
   if (l != 0) {
      errors++;
      printf
	  ("3: unpack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return failure if the block is too small to contain data */
   l = unpack_ToDoAppInfo(&mai, ToDoAppBlock, sizeof(ToDoAppBlock) - 1);
   if (l != 0) {
      errors++;
      printf
	  ("4: unpack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, 0);
   }

   /* Unpacker should return count of bytes used */
   l = unpack_ToDoAppInfo(&mai, ToDoAppBlock, sizeof(ToDoAppBlock));
   if (l != sizeof(ToDoAppBlock)) {
      errors++;
      printf
	  ("5: unpack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ToDoAppBlock));
   }

   if (
/*     strcmp(mai.category.name[0],"Unfiled") ||
     strcmp(mai.category.name[1],"Business") ||
     strcmp(mai.category.name[2],"Personal") ||
     strcmp(mai.category.name[3],"QuickList") ||
     strcmp(mai.category.name[4],"Foo") ||
     strcmp(mai.category.name[5],"") ||
     strcmp(mai.category.name[6],"") ||
     strcmp(mai.category.name[7],"") ||
     strcmp(mai.category.name[8],"") ||
     strcmp(mai.category.name[9],"") ||
     strcmp(mai.category.name[10],"") ||
     strcmp(mai.category.name[11],"") ||
     strcmp(mai.category.name[12],"") ||
     strcmp(mai.category.name[13],"") ||
     strcmp(mai.category.name[14],"") ||
     strcmp(mai.category.name[15],"") ||
     mai.category.renamed[0] ||
     mai.category.renamed[1] ||
     mai.category.renamed[2] ||
     mai.category.renamed[3] ||
     (!mai.category.renamed[4]) ||
     mai.category.renamed[5] ||
     mai.category.renamed[6] ||
     mai.category.renamed[7] ||
     mai.category.renamed[8] ||
     mai.category.renamed[9] ||
     mai.category.renamed[10] ||
     mai.category.renamed[11] ||
     mai.category.renamed[12] ||
     mai.category.renamed[13] ||
     mai.category.renamed[14] ||
     mai.category.renamed[15] ||
     (mai.category.ID[0] != 0) ||
     (mai.category.ID[1] != 1) ||
     (mai.category.ID[2] != 2) ||
     (mai.category.ID[3] != 3) ||
     (mai.category.ID[4] != 17) ||
     (mai.category.ID[5] != 0) ||
     (mai.category.ID[6] != 0) ||
     (mai.category.ID[7] != 0) ||
     (mai.category.ID[8] != 0) ||
     (mai.category.ID[9] != 0) ||
     (mai.category.ID[10] != 0) ||
     (mai.category.ID[11] != 0) ||
     (mai.category.ID[12] != 0) ||
     (mai.category.ID[13] != 0) ||
     (mai.category.ID[14] != 0) ||
     (mai.category.ID[15] != 0) ||
     (mai.category.lastUniqueID != 17)  ||*/
	 0) {
      errors++;
      printf("6: unpack_ToDoAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_ToDoAppInfo(&mai, 0, 0);
   if (l != sizeof(ToDoAppBlock)) {
      errors++;
      printf
	  ("7: pack_ToDoAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(ToDoAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_ToDoAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf("8: pack_ToDoAppInfo packed into too small buffer (got %d)\n",
	     l);
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, 1, "pack_ToDoAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_ToDoAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(ToDoAppBlock)) {
      errors++;
      printf
	  ("10: pack_ToDoAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ToDoAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(11, target, 8192, 128, l, "pack_ToDoAppInfo"))
      errors++;

   if (memcmp(target + 128, ToDoAppBlock, sizeof(ToDoAppBlock))) {
      errors++;
      printf
	  ("12: pack_ToDoAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(ToDoAppBlock, sizeof(ToDoAppBlock));
   }


   /* Unpacker should return count of bytes used */
   RecordBuffer = pi_buffer_new(sizeof(ToDoRecord));
   memcpy(RecordBuffer->data, ToDoRecord, RecordBuffer->allocated);
   RecordBuffer->used=sizeof(ToDoRecord);
   unpack_ToDo(&m, RecordBuffer, todo_v1);
   pi_buffer_free(RecordBuffer);

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("13: unpack_ToDo generated incorrect information\n");
   }

   reset_block(target, 8192);

   RecordBuffer = pi_buffer_new(0);
   if (pack_ToDo(&m, RecordBuffer, todo_v1) != 0) {
      errors++;
     printf("14: pack_ToDo returned failure\n");
   }

   if (RecordBuffer->used != sizeof(ToDoRecord)) {
      errors++;
      printf
	  ("15: pack_ToDo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ToDoRecord));
   }

   if (memcmp(RecordBuffer->data, ToDoRecord, sizeof(ToDoRecord))) {
      errors++;
      printf("16: pack_ToDo generated incorrect information. Got:\n");
      pi_dumpdata(RecordBuffer->data, l);
      printf(" expected:\n");
      pi_dumpdata(ToDoRecord, sizeof(ToDoRecord));
   }

   pi_buffer_free(RecordBuffer);

   printf("ToDo packers test completed with %d error(s).\n", errors);

   return errors;
}

char ExpenseAppBlock[24 * 16 + 8] = "\
\x00\x00\x55\x6e\x66\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x4e\x65\x77\x20\x59\x6f\x72\x6b\x00\x00\x00\x00\x00\x00\
\x00\x00\x50\x61\x72\x69\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x45\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x01\x02\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00";

/* Byte five is floating */
char ExpenseRecord[1 * 16 + 14] = "\
\xbb\x2a\x09\x01\x08\x00\x32\x39\x2e\x37\x32\x00\x55\x00\x43\x00\
\x41\x74\x74\x00\x54\x68\x65\x20\x6e\x6f\x74\x65\x2e\x00";


int test_expense()
{
   struct ExpenseAppInfo mai;
   struct Expense m;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l =
       unpack_ExpenseAppInfo(&mai, ExpenseAppBlock,
			     sizeof(ExpenseAppBlock) + 10);
   if (l != sizeof(ExpenseAppBlock)) {
      errors++;
      printf
	  ("1: unpack_ExpenseAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_ExpenseAppInfo(&mai, ExpenseAppBlock,
			     sizeof(ExpenseAppBlock) + 1);
   if (l != sizeof(ExpenseAppBlock)) {
      errors++;
      printf
	  ("2: unpack_ExpenseAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   /*l = unpack_ExpenseAppInfo(&mai, ExpenseAppBlock, sizeof(ExpenseAppBlock)-10);
      if (l != 0) {
      errors++;
      printf("x: unpack_ExpenseAppInfo returned incorrect length (got %d, expected %d)\n", l, 0);
      } */

   /* Unpacker should return count of bytes used */
   l =
       unpack_ExpenseAppInfo(&mai, ExpenseAppBlock,
			     sizeof(ExpenseAppBlock));
   if (l != sizeof(ExpenseAppBlock)) {
      errors++;
      printf
	  ("3: unpack_ExpenseAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseAppBlock));
   }


   if (
/*     strcmp(mai.category.name[0],"Unfiled") ||
     strcmp(mai.category.name[1],"Business") ||
     strcmp(mai.category.name[2],"Personal") ||
     strcmp(mai.category.name[3],"QuickList") ||
     strcmp(mai.category.name[4],"Foo") ||
     strcmp(mai.category.name[5],"") ||
     strcmp(mai.category.name[6],"") ||
     strcmp(mai.category.name[7],"") ||
     strcmp(mai.category.name[8],"") ||
     strcmp(mai.category.name[9],"") ||
     strcmp(mai.category.name[10],"") ||
     strcmp(mai.category.name[11],"") ||
     strcmp(mai.category.name[12],"") ||
     strcmp(mai.category.name[13],"") ||
     strcmp(mai.category.name[14],"") ||
     strcmp(mai.category.name[15],"") ||
     mai.category.renamed[0] ||
     mai.category.renamed[1] ||
     mai.category.renamed[2] ||
     mai.category.renamed[3] ||
     (!mai.category.renamed[4]) ||
     mai.category.renamed[5] ||
     mai.category.renamed[6] ||
     mai.category.renamed[7] ||
     mai.category.renamed[8] ||
     mai.category.renamed[9] ||
     mai.category.renamed[10] ||
     mai.category.renamed[11] ||
     mai.category.renamed[12] ||
     mai.category.renamed[13] ||
     mai.category.renamed[14] ||
     mai.category.renamed[15] ||
     (mai.category.ID[0] != 0) ||
     (mai.category.ID[1] != 1) ||
     (mai.category.ID[2] != 2) ||
     (mai.category.ID[3] != 3) ||
     (mai.category.ID[4] != 17) ||
     (mai.category.ID[5] != 0) ||
     (mai.category.ID[6] != 0) ||
     (mai.category.ID[7] != 0) ||
     (mai.category.ID[8] != 0) ||
     (mai.category.ID[9] != 0) ||
     (mai.category.ID[10] != 0) ||
     (mai.category.ID[11] != 0) ||
     (mai.category.ID[12] != 0) ||
     (mai.category.ID[13] != 0) ||
     (mai.category.ID[14] != 0) ||
     (mai.category.ID[15] != 0) ||
     (mai.category.lastUniqueID != 17)  ||*/
	 0) {
      errors++;
      printf("4: unpack_ExpenseAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_ExpenseAppInfo(&mai, 0, 0);
   if (l != sizeof(ExpenseAppBlock)) {
      errors++;
      printf
	  ("5: pack_ExpenseAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(ExpenseAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_ExpenseAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf
	  ("6: pack_ExpenseAppInfo packed into too small buffer (got %d)\n",
	   l);
   }

   /* Packer should not scribble on memory */
   if (check_block(7, target, 8192, 128, 1, "pack_ExpenseAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_ExpenseAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(ExpenseAppBlock)) {
      errors++;
      printf
	  ("8: pack_ExpenseAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, l, "pack_ExpenseAppInfo"))
      errors++;

   if (memcmp(target + 128, ExpenseAppBlock, sizeof(ExpenseAppBlock))) {
      errors++;
      printf
	  ("10: pack_ExpenseAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(ExpenseAppBlock, sizeof(ExpenseAppBlock));
   }


   /* Unpacker should return count of bytes used */
   l = unpack_Expense(&m, ExpenseRecord, sizeof(ExpenseRecord) + 10);
   if (l != sizeof(ExpenseRecord)) {
      errors++;
      printf
	  ("11: unpack_Expense returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseRecord));
   }

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("12: unpack_Expense generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_Expense(&m, 0, 0);
   if (l != sizeof(ExpenseRecord)) {
      errors++;
      printf
	  ("13: pack_Expense returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(ExpenseRecord));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_Expense(&m, target + 128, 1);
   if (l != 0) {
      errors++;
      printf("14: pack_Expense packed into too small buffer (got %d)\n",
	     l);
   }

   /* Packer should not scribble on memory */
   if (check_block(15, target, 8192, 128, 1, "pack_Expense"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_Expense(&m, target + 128, 8192 - 256);
   if (l != sizeof(ExpenseRecord)) {
      errors++;
      printf
	  ("16: pack_Expense returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(ExpenseRecord));
   }

   /* Packer should not scribble on memory */
   if (check_block(17, target, 8192, 128, l, "pack_Expense"))
      errors++;

   if (memcmp(target + 128, ExpenseRecord, sizeof(ExpenseRecord))) {
      errors++;
      printf("18: pack_Expense generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(ExpenseRecord, sizeof(ExpenseRecord));
   }

   printf("Expense packers test completed with %d error(s).\n", errors);

   return errors;
}

char MailAppBlock[18 * 16 + 1] = "\
\x00\x1f\x49\x6e\x62\x6f\x78\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x4f\x75\x74\x62\x6f\x78\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x44\x65\x6c\x65\x74\x65\x64\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x46\x69\x6c\x65\x64\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x44\x72\x61\x66\x74\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\
\x00\x00\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\
\x0e\x0f\x0f\x00\x00\x00\xff\xff\x00\x00\x00\x00\x00\x00\x01\x20\
\x00";

char MailRecord[1 * 16 + 2] = "\
\x00\x00\x00\x00\x78\x00\x43\x00\x00\x61\x00\x62\x00\x00\x00\x00\
\x44\x00";			/*\x27";  This byte seems to be spurious */

char MailSigPreference[3] = "\
\x61\x62\x00";

char MailSyncPreference[13] = "\
\x02\x01\x00\x00\x17\x70\x61\x74\x00\x6c\x64\x00\x00";

int test_mail()
{
   struct MailAppInfo mai;
   struct Mail m;
   struct MailSyncPref s1;
   struct MailSignaturePref s2;
   int l;
   int errors = 0;

   /* Unpacker should return count of bytes used */
   l = unpack_MailAppInfo(&mai, MailAppBlock, sizeof(MailAppBlock) + 10);
   if (l != sizeof(MailAppBlock)) {
      errors++;
      printf
	  ("1: unpack_MailAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailAppBlock));
   }

   /* Unpacker should return count of bytes used */
   l = unpack_MailAppInfo(&mai, MailAppBlock, sizeof(MailAppBlock) + 1);
   if (l != sizeof(MailAppBlock)) {
      errors++;
      printf
	  ("2: unpack_MailAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailAppBlock));
   }

   /* Unpacker should return failure if the block is too small to contain data */
   /*l = unpack_MailAppInfo(&mai, MailAppBlock, sizeof(MailAppBlock)-10);
      if (l != 0) {
      errors++;
      printf("2: unpack_MailAppInfo returned incorrect length (got %d, expected %d)\n", l, 0);
      } */

   /* Unpacker should return count of bytes used */
   l = unpack_MailAppInfo(&mai, MailAppBlock, sizeof(MailAppBlock));
   if (l != sizeof(MailAppBlock)) {
      errors++;
      printf
	  ("3: unpack_MailAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailAppBlock));
   }


   if (
/*     strcmp(mai.category.name[0],"Unfiled") ||
     strcmp(mai.category.name[1],"Business") ||
     strcmp(mai.category.name[2],"Personal") ||
     strcmp(mai.category.name[3],"QuickList") ||
     strcmp(mai.category.name[4],"Foo") ||
     strcmp(mai.category.name[5],"") ||
     strcmp(mai.category.name[6],"") ||
     strcmp(mai.category.name[7],"") ||
     strcmp(mai.category.name[8],"") ||
     strcmp(mai.category.name[9],"") ||
     strcmp(mai.category.name[10],"") ||
     strcmp(mai.category.name[11],"") ||
     strcmp(mai.category.name[12],"") ||
     strcmp(mai.category.name[13],"") ||
     strcmp(mai.category.name[14],"") ||
     strcmp(mai.category.name[15],"") ||
     mai.category.renamed[0] ||
     mai.category.renamed[1] ||
     mai.category.renamed[2] ||
     mai.category.renamed[3] ||
     (!mai.category.renamed[4]) ||
     mai.category.renamed[5] ||
     mai.category.renamed[6] ||
     mai.category.renamed[7] ||
     mai.category.renamed[8] ||
     mai.category.renamed[9] ||
     mai.category.renamed[10] ||
     mai.category.renamed[11] ||
     mai.category.renamed[12] ||
     mai.category.renamed[13] ||
     mai.category.renamed[14] ||
     mai.category.renamed[15] ||
     (mai.category.ID[0] != 0) ||
     (mai.category.ID[1] != 1) ||
     (mai.category.ID[2] != 2) ||
     (mai.category.ID[3] != 3) ||
     (mai.category.ID[4] != 17) ||
     (mai.category.ID[5] != 0) ||
     (mai.category.ID[6] != 0) ||
     (mai.category.ID[7] != 0) ||
     (mai.category.ID[8] != 0) ||
     (mai.category.ID[9] != 0) ||
     (mai.category.ID[10] != 0) ||
     (mai.category.ID[11] != 0) ||
     (mai.category.ID[12] != 0) ||
     (mai.category.ID[13] != 0) ||
     (mai.category.ID[14] != 0) ||
     (mai.category.ID[15] != 0) ||
     (mai.category.lastUniqueID != 17)  ||*/
	 0) {
      errors++;
      printf("4: unpack_MailAppInfo generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_MailAppInfo(&mai, 0, 0);
   if (l != sizeof(MailAppBlock)) {
      errors++;
      printf
	  ("5: pack_MailAppInfo returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(MailAppBlock));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_MailAppInfo(&mai, target + 128, 1);
   if (l != 0) {
      errors++;
      printf("6: pack_MailAppInfo packed into too small buffer (got %d)\n",
	     l);
   }

   /* Packer should not scribble on memory */
   if (check_block(7, target, 8192, 128, 1, "pack_MailAppInfo"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_MailAppInfo(&mai, target + 128, 8192 - 256);
   if (l != sizeof(MailAppBlock)) {
      errors++;
      printf
	  ("8: pack_MailAppInfo returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailAppBlock));
   }

   /* Packer should not scribble on memory */
   if (check_block(9, target, 8192, 128, l, "pack_MailAppInfo"))
      errors++;

   if (memcmp(target + 128, MailAppBlock, sizeof(MailAppBlock))) {
      errors++;
      printf
	  ("10: pack_MailAppInfo generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MailAppBlock, sizeof(MailAppBlock));
   }


   /* Unpacker should return count of bytes used */
   l = unpack_Mail(&m, MailRecord, sizeof(MailRecord) + 10);
   if (l != sizeof(MailRecord)) {
      errors++;
      printf
	  ("11: unpack_Mail returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailRecord));
   }

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("12: unpack_Mail generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_Mail(&m, 0, 0);
   if (l != sizeof(MailRecord)) {
      errors++;
      printf
	  ("13: pack_Mail returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(MailRecord));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_Mail(&m, target + 128, 1);
   if (l != 0) {
      errors++;
      printf("14: pack_Mail packed into too small buffer (got %d)\n", l);
   }

   /* Packer should not scribble on memory */
   if (check_block(15, target, 8192, 128, 1, "pack_Mail"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_Mail(&m, target + 128, 8192 - 256);
   if (l != sizeof(MailRecord)) {
      errors++;
      printf
	  ("16: pack_Mail returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailRecord));
   }

   /* Packer should not scribble on memory */
   if (check_block(17, target, 8192, 128, l, "pack_Mail"))
      errors++;

   if (memcmp(target + 128, MailRecord, sizeof(MailRecord))) {
      errors++;
      printf("18: pack_Mail generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MailRecord, sizeof(MailRecord));
   }


   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSyncPref(&s1, MailSyncPreference,
			   sizeof(MailSyncPreference) + 10);
   if (l != sizeof(MailSyncPreference)) {
      errors++;
      printf
	  ("19: unpack_MailSyncPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSyncPref(&s1, MailSyncPreference,
			   sizeof(MailSyncPreference) + 1);
   if (l != sizeof(MailSyncPreference)) {
      errors++;
      printf
	  ("20: unpack_MailSyncPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSyncPref(&s1, MailSyncPreference,
			   sizeof(MailSyncPreference));
   if (l != sizeof(MailSyncPreference)) {
      errors++;
      printf
	  ("21: unpack_MailSyncPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("22: unpack_MailSyncPref generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_MailSyncPref(&s1, 0, 0);
   if (l != sizeof(MailSyncPreference)) {
      errors++;
      printf
	  ("23: pack_MailSyncPref returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_MailSyncPref(&s1, target + 128, 1);
   if (l != 0) {
      errors++;
      printf
	  ("24: pack_MailSyncPref packed into too small buffer (got %d)\n",
	   l);
   }

   /* Packer should not scribble on memory */
   if (check_block(25, target, 8192, 128, 1, "pack_MailSyncPref"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_MailSyncPref(&s1, target + 128, 8192 - 256);
   if (l != sizeof(MailSyncPreference)) {
      errors++;
      printf
	  ("26: pack_MailSyncPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   /* Packer should not scribble on memory */
   if (check_block(27, target, 8192, 128, l, "pack_Mail"))
      errors++;

   if (memcmp
       (target + 128, MailSyncPreference, sizeof(MailSyncPreference))) {
      errors++;
      printf
	  ("28: pack_MailSyncPref generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MailSyncPreference, sizeof(MailSyncPreference));
   }


   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSignaturePref(&s2, MailSigPreference,
				sizeof(MailSigPreference) + 10);
   if (l != sizeof(MailSigPreference)) {
      errors++;
      printf
	  ("29: unpack_MailSigPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSignaturePref(&s2, MailSigPreference,
				sizeof(MailSigPreference) + 1);
   if (l != sizeof(MailSigPreference)) {
      errors++;
      printf
	  ("30: unpack_MailSigPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   /* Unpacker should return count of bytes used */
   l =
       unpack_MailSignaturePref(&s2, MailSigPreference,
				sizeof(MailSigPreference));
   if (l != sizeof(MailSigPreference)) {
      errors++;
      printf
	  ("31: unpack_MailSigPref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSyncPreference));
   }

   if (
/*     (m.phonelabel[0] != 0) ||
     (m.phonelabel[1] != 1) ||
     (m.phonelabel[2] != 2) ||
     (m.phonelabel[3] != 3) ||
     (m.phonelabel[4] != 4) ||
     strcmp(m.entry[0],"Shaw") ||
     strcmp(m.entry[1],"Bernard") ||
     m.entry[2] ||
     m.entry[3] ||
     strcmp(m.entry[8],"None known") ||
     strcmp(m.entry[14],"C1") ||
     (m.whichphone != 1) ||*/
	 0) {
      errors++;
      printf("32: unpack_MailSyncPref generated incorrect information\n");
   }

   /* Packer should return necessary block length when no buffer is given */
   l = pack_MailSignaturePref(&s2, 0, 0);
   if (l != sizeof(MailSigPreference)) {
      errors++;
      printf
	  ("33: pack_MailSignaturePref returned incorrect allocation length (got %d, expected %d)\n",
	   l, sizeof(MailSigPreference));
   }

   reset_block(target, 8192);

   /* Packer should not pack when the block length is too small */
   l = pack_MailSignaturePref(&s2, target + 128, 1);
   if (l != 0) {
      errors++;
      printf
	  ("34: pack_MailSignaturePref packed into too small buffer (got %d)\n",
	   l);
   }

   /* Packer should not scribble on memory */
   if (check_block(35, target, 8192, 128, 1, "pack_MailSyncPref"))
      errors++;

   reset_block(target, 8192);

   /* Packer should return length of data written */
   l = pack_MailSignaturePref(&s2, target + 128, 8192 - 256);
   if (l != sizeof(MailSigPreference)) {
      errors++;
      printf
	  ("36: pack_MailSignaturePref returned incorrect length (got %d, expected %d)\n",
	   l, sizeof(MailSigPreference));
   }

   /* Packer should not scribble on memory */
   if (check_block(37, target, 8192, 128, l, "pack_Mail"))
      errors++;

   if (memcmp(target + 128, MailSigPreference, sizeof(MailSigPreference))) {
      errors++;
      printf
	  ("38: pack_MailSignaturePref generated incorrect information. Got:\n");
      pi_dumpdata(target + 128, l);
      printf(" expected:\n");
      pi_dumpdata(MailSigPreference, sizeof(MailSigPreference));
   }

   printf("Mail packers test completed with %d error(s).\n", errors);

   return errors;
}


int main(int argc, char *argv[])
{
   seed = time(0) & 0xff;	/* Make scribble checker use a random check value */
   target = malloc(8192);
   targetlen = 8192;

   test_memo();
   test_address();
   test_appointment();
   test_todo();
   test_expense();
   test_mail();
   return 0;
}
