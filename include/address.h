#ifndef _PILOT_ADDRESS_H_
#define _PILOT_ADDRESS_H_

struct Address {
  int phonelabel1, phonelabel2, phonelabel3, phonelabel4, phonelabel5;
  int whichphone;
  
  char *lastname, *firstname, *company;
  char *phone1, *phone2, *phone3, *phone4, *phone5;
  char *address, *city, *state, *zip, *country, *title;
  char *custom1, *custom2, *custom3, *custom4;
  char * note;
};

void free_Address(struct Address *);
void unpack_Address(struct Address *, unsigned char * record, int len);
void pack_Address(struct Address *, unsigned char * record, int * len);


#endif /* _PILOT_ADDRESS_H_ */
