#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pi-file.h"
#include "pi-versamail.h"

size_t palm_strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

void hex_dump(unsigned char *buf, int size)
{
	char text[18];
	int i;

	bzero(text, 17);

	for (i = 0; i < size; i++) {
		printf("%02x ", buf[i]);
		text[i % 16] = (isprint(buf[i]) ? buf[i] : '.');
		if ((i + 1) % 16 == 0) {
			printf("%s\n", text);
			bzero(text, 17);
		}
	}
	printf("\n");
}

int print_cat_info(struct CategoryAppInfo *ai)
{
	int i;

	printf("Category Info Start\n");
	printf("renamed=");
	for (i = 0; i < 16; i++) {
		printf("%d", ai->renamed[i]);
	}
	printf("\n");
	for (i = 0; i < 16; i++) {
		printf("Index %02d ID %03d %s\n", i, ai->ID[i],
		       ai->name[i]);
	}
	printf("lastUniqueID = %d\n", ai->lastUniqueID);
	printf("Category Info End\n");
	return 0;
}

int print_versamail_app_info(int db, void *record, int size)
{
	struct VersaMailAppInfo ai;

	printf("Category app info\n");	//undo
	//hex_dump(record, size);//undo
	unpack_VersaMailAppInfo(&ai, record, size);
	print_cat_info(&(ai.category));

	return 0;
}


void print_versamail_record(void *record, int size, int attr, int idx)
{
	struct VersaMail mail;
	char datestr[255];

	unpack_VersaMail(&mail, record, size);

	printf("----\n");
	printf("version: %lu\n", mail.imapuid);
	palm_strftime(datestr, 254, "%c", &(mail.date));
	//printf("date: %s\n", datestr);
	//printf("category: %d\n", mail.category);
	//printf("account: %d\n", mail.accountNo);
	printf("unknown1: %d\n", mail.unknown1);
	printf("unknown2: %d\n", mail.unknown2);
	printf("reserved1: %d\n", mail.reserved1);
	printf("reserved2: %d\n", mail.reserved2);
	printf("download: %d\n", mail.download);
	printf("mark: %d\n", mail.mark);
	printf("read: %d\n", mail.read);
	printf("body: %s\n", mail.body);
	//printf("to: %s\n", mail.to);
	//printf("from: %s\n", mail.from);
	//printf("cc: %s\n", mail.cc);
	//printf("bcc: %s\n", mail.bcc);
	printf("subject: %s\n", mail.subject);
	//printf("dateString: %s\n", mail.dateString);
	//printf("replyTo: %s\n", mail.replyTo);

	free_VersaMail(&mail);
}

void validate_versamail_packer(void *record, int size, int attr, int idx)
{
	struct VersaMail mail;
	char *buffer;
	int len;
	int i;

	unpack_VersaMail(&mail, record, size);
	len = pack_VersaMail(&mail, NULL, 0);
	buffer = malloc(len);
	pack_VersaMail(&mail, buffer, len);

	/*  printf("-------------\n"); */
	if (size - len != 0) {
		printf
		    ("on-disk size is %d, pack asked for %d to repack, wrong by %d.\n",
		     size, len, size - len);
	}
	printf("subject: %s\n", mail.subject);
	printf("  attachment count: %d\n", mail.attachmentCount);

	if (0) {
		printf("uid: %d\n", (int) mail.imapuid);
		printf("size: %d\n", mail.msgSize);
		printf("unknown1: %d\n", mail.unknown1);
		printf("unknown2: %d\n", mail.unknown2);
		printf("reserved1: %d\n", mail.reserved1);
		printf("reserved2: %d\n", mail.reserved2);
		printf("download: %d\n", mail.download);
		printf("mark: %d\n", mail.mark);
		/*
		   printf("body: %s\n", mail.body);
		   printf("dateString: %s\n", mail.dateString);
		   printf("replyTo: %s\n", mail.replyTo);
		 */
	}

	if (0) {
		if (mail.unknown3length > 0) {
			for (i = 0; i < mail.unknown3length; i++) {
				printf
				    ("unknown attachment system Byte %3d: 0x%10x | %c | %6d\n",
				     i, ((char *) mail.unknown3)[i],
				     (isprint(((char *) mail.unknown3)[i])
				      ? ((char *) mail.unknown3)[i] : '.'),
				     ((char *) mail.unknown3)[i]);
			}
		}
	}


	for (i = 0; i < ((size) > (len) ? (size) : (len)); i++) {
		if ((i < len) && (i < size)) {
			if (!((char *) record)[i] == buffer[i]) {
				printf
				    ("WRONG Byte %3d: 0x%10x (%c) vs. 0x%10x (%c)\n",
				     i, ((char *) record)[i],
				     (isprint(((char *) record)[i])
				      ? ((char *) record)[i] : '.'),
				     buffer[i],
				     (isprint(buffer[i]) ? buffer[i] :
				      '.'));
			} else {
				if (0) {
					printf
					    ("  OK  Byte %3d: 0x%10x (%c) vs. 0x%10x (%c)\n",
					     i, ((char *) record)[i],
					     (isprint(((char *) record)[i])
					      ? ((char *) record)[i] :
					      '.'), buffer[i],
					     (isprint(buffer[i]) ?
					      buffer[i] : '.'));
				}
			}
		} else if (i < len) {
			printf
			    ("WRONG Byte %3d: ----------- vs. 0x%10x (%c)\n",
			     i, buffer[i],
			     (isprint(buffer[i]) ? buffer[i] : '.'));
		} else if (i < size) {
			printf
			    ("WRONG Byte %3d: 0x%10x (%c) vs. -----------\n",
			     i + 1, ((char *) record)[i],
			     (isprint(((char *) record)[i])
			      ? ((char *) record)[i] : '.'));
		}
	}
	free(buffer);
	free_VersaMail(&mail);

}

int main(int argc, char *argv[])
{
	struct pi_file *pi_fp;
	char *DBname;
	int r;
	int idx;
	size_t size;
	int attr;
	int cat;
	pi_uid_t uid;
	void *record;

	DBname = "MultiMail Messages.pdb";

	pi_fp = pi_file_open(DBname);
	if (pi_fp == 0) {
		printf("Unable to open '%s'!\n", DBname);
		return -1;
	}

	pi_file_get_app_info PI_ARGS((pi_fp, &record, &size));

	print_versamail_app_info(r, record, size);
	printf("----------\n");

	for (idx = 0;; idx++) {
		r = pi_file_read_record(pi_fp, idx, &record, &size, &attr,
					&cat, &uid);
		if (r < 0)
			break;
		//print_versamail_record(record, size, attr, idx);
		validate_versamail_packer(record, size, attr, idx);
	}

	pi_file_close(pi_fp);

	return 0;
}
