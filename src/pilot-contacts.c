/*
 * $Id$
 *
 * pilot-contacts.c:  Read/write Palm OS 5 ContactsDB-PAdd in vCard 3.0
 *
 * Copyright (c) 2026, David A. Desrosiers (desrod@gnu-designs.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "pi-contact.h"
#include "pi-file.h"
#include "pi-header.h"
#include "pi-source.h"
#include "pi-userland.h"

/* Maximum length of one unfolded vCard property line                  */
#define VCARD_LINE_MAX 8192

/* Column at which to fold long vCard output lines (RFC 6350)          */
#define VCARD_FOLD_AT 75

/* Buffer size for a single escaped/unescaped property value           */
#define VCARD_VAL_MAX 4096

enum {
  mode_none = 0,
  mode_write, /* Palm  → vCard file  */
  mode_read,  /* vCard file → Palm   */
  mode_list   /* Palm  → stdout      */
};

/* Palm phone label index → vCard TYPE string.
 * Index 4 (Email) is NULL: emitted as EMAIL property, not TEL.       */
static const char *phone_vctypes[] = {
    "WORK,VOICE", /* 0: Work   */
    "HOME,VOICE", /* 1: Home   */
    "FAX",        /* 2: Fax    */
    "OTHER",      /* 3: Other  */
    NULL,         /* 4: Email  */
    "PREF,VOICE", /* 5: Main   */
    "PAGER",      /* 6: Pager  */
    "CELL"        /* 7: Mobile */
};

/* Palm IM label index → vCard X- property name                        */
static const char *im_xprops[] = {
    "X-JABBER", /* 0: generic */
    "X-AIM",    /* 1          */
    "X-MSN",    /* 2          */
    "X-YAHOO",  /* 3          */
    "X-ICQ"     /* 4          */
};

/* Address slot base indices within Contact.entry[]                    */
static const int addr_base[] = {contAddress1, contAddress2, contAddress3};

#ifdef HAVE_ICONV
/* Charset conversion descriptors, opened in main() when --charset is
 * given.  (iconv_t)-1 means no conversion (UTF-8 passthrough).       */
static iconv_t cd_to_utf8 = (iconv_t)-1;   /* Palm enc → UTF-8  */
static iconv_t cd_from_utf8 = (iconv_t)-1; /* UTF-8 → Palm enc  */
#endif

/* ------------------------------------------------------------------ */
/* String helpers                                                       */
/* ------------------------------------------------------------------ */

/***********************************************************************
 *
 * Function:	ci_strstr
 *
 * Summary:	Case-insensitive substring search.  Portable substitute
 *		for strcasestr(3), which is not in C23.
 *
 * Parameters:	haystack, needle
 *
 * Returns:	Pointer to first match in haystack, or NULL
 *
 ***********************************************************************/
static const char *ci_strstr(const char *haystack, const char *needle) {
  size_t nlen = strlen(needle);

  if (nlen == 0) return haystack;

  for (; *haystack; haystack++)
    if (strncasecmp(haystack, needle, nlen) == 0) return haystack;

  return NULL;
}

/***********************************************************************
 *
 * Function:	escape_value
 *
 * Summary:	Copy src into dst (size dstsz), escaping backslash,
 *		comma, semicolon, and newline per vCard 3.0.
 *
 * Parameters:	dst, dstsz, src
 *
 * Returns:	dst
 *
 ***********************************************************************/
static char *escape_value(char *dst, size_t dstsz, const char *src) {
  char *d = dst;
  char *end = dst + dstsz - 2; /* need room for 2-char escape + NUL */

  if (src == NULL || dstsz == 0) {
    if (dstsz > 0) dst[0] = '\0';
    return dst;
  }

  for (; *src && d <= end; src++) {
    switch (*src) {
      case '\\':
        *d++ = '\\';
        *d++ = '\\';
        break;
      case ',':
        *d++ = '\\';
        *d++ = ',';
        break;
      case ';':
        *d++ = '\\';
        *d++ = ';';
        break;
      case '\n':
        *d++ = '\\';
        *d++ = 'n';
        break;
      case '\r': /* drop bare CR */
        break;
      default:
        *d++ = *src;
        break;
    }
  }
  *d = '\0';
  return dst;
}

/***********************************************************************
 *
 * Function:	unescape_value
 *
 * Summary:	In-place unescape of a vCard 3.0 property value.
 *		\\→\ \,→, \;→; \n→newline
 *
 * Parameters:	s — string modified in place
 *
 * Returns:	s
 *
 ***********************************************************************/
static char *unescape_value(char *s) {
  char *r = s, *w = s;

  for (; *r; r++) {
    if (*r == '\\' && *(r + 1)) {
      r++;
      switch (*r) {
        case '\\':
          *w++ = '\\';
          break;
        case ',':
          *w++ = ',';
          break;
        case ';':
          *w++ = ';';
          break;
        case 'n': /* fall through */
        case 'N':
          *w++ = '\n';
          break;
        default:
          *w++ = *r;
          break;
      }
    } else {
      *w++ = *r;
    }
  }
  *w = '\0';
  return s;
}

/* ------------------------------------------------------------------ */
/* Charset conversion helpers (require HAVE_ICONV)                     */
/* ------------------------------------------------------------------ */

#ifdef HAVE_ICONV
/***********************************************************************
 *
 * Function:	conv_str
 *
 * Summary:	Convert string src using iconv descriptor cd into dst
 *		(size dstsz).  Resets the iconv state before each call.
 *		If cd is invalid or conversion fails, copies src unchanged.
 *
 *		The iconv() input argument is typed differently on macOS
 *		(const char **) vs POSIX (char **).  We use void* to
 *		satisfy both without warnings.
 *
 * Parameters:	cd, src, dst, dstsz
 *
 * Returns:	Nothing
 *
 ***********************************************************************/
static void conv_str(iconv_t cd, const char *src, char *dst, size_t dstsz) {
  const char *in;
  char *out;
  size_t srclen, dstlen;

  if (cd == (iconv_t)-1 || src == NULL || dstsz == 0) {
    if (dst && dstsz > 0) {
      strncpy(dst, src ? src : "", dstsz - 1);
      dst[dstsz - 1] = '\0';
    }
    return;
  }

  iconv(cd, NULL, NULL, NULL, NULL); /* reset shift state */

  in = src;
  srclen = strlen(src);
  out = dst;
  dstlen = dstsz - 1;

  if (iconv(cd, (void *)&in, &srclen, &out, &dstlen) == (size_t)-1) {
    /* Conversion failed: pass through unchanged               */
    strncpy(dst, src, dstsz - 1);
    dst[dstsz - 1] = '\0';
  } else {
    *out = '\0';
  }
}

/***********************************************************************
 *
 * Function:	conv_escape
 *
 * Summary:	Convert src from the Palm's character encoding to UTF-8
 *		(if a conversion descriptor was opened for --charset),
 *		then escape the result for vCard output into dst.
 *		When no charset conversion is active this is identical
 *		to escape_value().
 *
 * Parameters:	dst, dstsz, src
 *
 * Returns:	dst
 *
 ***********************************************************************/
static char *conv_escape(char *dst, size_t dstsz, const char *src) {
  char conv[VCARD_VAL_MAX];

  if (cd_to_utf8 != (iconv_t)-1 && src && src[0]) {
    conv_str(cd_to_utf8, src, conv, sizeof(conv));
    return escape_value(dst, dstsz, conv);
  }
  return escape_value(dst, dstsz, src);
}
#else
/* No iconv: conv_escape is just escape_value                          */
#define conv_escape(dst, dstsz, src) escape_value((dst), (dstsz), (src))
#endif /* HAVE_ICONV */

/* ------------------------------------------------------------------ */
/* vCard output                                                         */
/* ------------------------------------------------------------------ */

/***********************************************************************
 *
 * Function:	vcard_emit
 *
 * Summary:	Print one complete vCard property line to f, folding at
 *		VCARD_FOLD_AT columns (CRLF + one space) as required by
 *		vCard 3.0 / RFC 6350.
 *
 *		prop  — property name and parameters including the colon,
 *		        e.g. "TEL;TYPE=CELL:"
 *		value — already-escaped property value (may be NULL/"")
 *
 * Parameters:	f, prop, value
 *
 * Returns:	Nothing
 *
 ***********************************************************************/
static void vcard_emit(FILE *f, const char *prop, const char *value) {
  char line[VCARD_LINE_MAX];
  int len, col, i;

  len = snprintf(line, sizeof(line), "%s%s", prop, value ? value : "");
  if (len < 0) return;
  if (len >= (int)sizeof(line)) len = (int)sizeof(line) - 1;

  col = 0;
  for (i = 0; i < len; i++) {
    if (col >= VCARD_FOLD_AT) {
      fputs("\r\n ", f);
      col = 1;
    }
    fputc(line[i], f);
    col++;
  }
  fputs("\r\n", f);
}

/***********************************************************************
 *
 * Function:	write_vcard
 *
 * Summary:	Write one Contact record to f as a vCard 3.0 block.
 *
 * Parameters:	f, c, cai, category
 *
 * Returns:	Nothing
 *
 ***********************************************************************/
static void write_vcard(FILE *f, const struct Contact *c,
                        const struct ContactAppInfo *cai, int category) {
  char prop[128];
  char esc_a[VCARD_VAL_MAX];
  char esc_b[VCARD_VAL_MAX];
  char adr[VCARD_LINE_MAX];
  char s_street[512], s_city[512], s_state[256];
  char s_zip[128], s_country[256];
  const char *last, *first;
  int i, k;

  fputs("BEGIN:VCARD\r\n", f);
  fputs("VERSION:3.0\r\n", f);

  /* N:Last;First;;;                                                 */
  last = c->entry[contLastname] ? c->entry[contLastname] : "";
  first = c->entry[contFirstname] ? c->entry[contFirstname] : "";
  snprintf(prop, sizeof(prop), "%s;%s;;;",
           conv_escape(esc_a, sizeof(esc_a), last),
           conv_escape(esc_b, sizeof(esc_b), first));
  vcard_emit(f, "N:", prop);

  /* FN: — display name, falling back to company                    */
  if (*first || *last) {
    if (*first && *last)
      snprintf(prop, sizeof(prop), "%s %s", first, last);
    else
      snprintf(prop, sizeof(prop), "%s%s", first, last);
    vcard_emit(f, "FN:", conv_escape(esc_a, sizeof(esc_a), prop));
  } else if (c->entry[contCompany] && c->entry[contCompany][0]) {
    vcard_emit(f,
               "FN:", conv_escape(esc_a, sizeof(esc_a), c->entry[contCompany]));
  } else {
    vcard_emit(f, "FN:", "");
  }

  /* ORG: */
  if (c->entry[contCompany] && c->entry[contCompany][0])
    vcard_emit(
        f, "ORG:", conv_escape(esc_a, sizeof(esc_a), c->entry[contCompany]));

  /* TITLE: */
  if (c->entry[contTitle] && c->entry[contTitle][0])
    vcard_emit(
        f, "TITLE:", conv_escape(esc_a, sizeof(esc_a), c->entry[contTitle]));

  /* Phone slots 0–6 (contPhone1 … contPhone7)                      */
  for (i = 0; i < 7; i++) {
    int idx = contPhone1 + i;
    int lbl = c->phoneLabel[i];
    const char *val = c->entry[idx];

    if (val == NULL || val[0] == '\0') continue;

    if (lbl == 4) {
      /* Email: emit as EMAIL property                   */
      vcard_emit(
          f, "EMAIL;TYPE=INTERNET:", conv_escape(esc_a, sizeof(esc_a), val));
    } else {
      const char *vct;
      if (lbl >= 0 &&
          lbl < (int)(sizeof(phone_vctypes) / sizeof(phone_vctypes[0])))
        vct = phone_vctypes[lbl];
      else
        vct = NULL;
      if (vct == NULL) vct = "OTHER";
      snprintf(prop, sizeof(prop), "TEL;TYPE=%s:", vct);
      vcard_emit(f, prop, conv_escape(esc_a, sizeof(esc_a), val));
    }
  }

  /* IM slots 0–1 (contIM1, contIM2)                                */
  for (i = 0; i < 2; i++) {
    int idx = contIM1 + i;
    int lbl = c->IMLabel[i];
    const char *val = c->entry[idx];

    if (val == NULL || val[0] == '\0') continue;
    if (lbl < 0 || lbl >= (int)(sizeof(im_xprops) / sizeof(im_xprops[0])))
      lbl = 0;
    snprintf(prop, sizeof(prop), "%s:", im_xprops[lbl]);
    vcard_emit(f, prop, conv_escape(esc_a, sizeof(esc_a), val));
  }

  /* URL: */
  if (c->entry[contWebsite] && c->entry[contWebsite][0])
    vcard_emit(
        f, "URL:", conv_escape(esc_a, sizeof(esc_a), c->entry[contWebsite]));

  /* Address blocks (up to 3): ADR;TYPE=…:;;Street;City;State;Zip;Country */
  for (k = 0; k < 3; k++) {
    int base = addr_base[k];
    int albl = c->addressLabel[k];
    const char *atype;

    const char *street = c->entry[base] ? c->entry[base] : "";
    const char *city = c->entry[base + 1] ? c->entry[base + 1] : "";
    const char *state = c->entry[base + 2] ? c->entry[base + 2] : "";
    const char *zip = c->entry[base + 3] ? c->entry[base + 3] : "";
    const char *country = c->entry[base + 4] ? c->entry[base + 4] : "";

    if (!*street && !*city && !*state && !*zip && !*country) continue;

    atype = (albl == 1) ? "HOME" : (albl == 2) ? "OTHER" : "WORK";

    conv_escape(s_street, sizeof(s_street), street);
    conv_escape(s_city, sizeof(s_city), city);
    conv_escape(s_state, sizeof(s_state), state);
    conv_escape(s_zip, sizeof(s_zip), zip);
    conv_escape(s_country, sizeof(s_country), country);

    snprintf(adr, sizeof(adr), ";;%s;%s;%s;%s;%s", s_street, s_city, s_state,
             s_zip, s_country);
    snprintf(prop, sizeof(prop), "ADR;TYPE=%s:", atype);
    vcard_emit(f, prop, adr);
  }

  /* NOTE: */
  if (c->entry[contNote] && c->entry[contNote][0])
    vcard_emit(f,
               "NOTE:", conv_escape(esc_a, sizeof(esc_a), c->entry[contNote]));

  /* BDAY: */
  if (c->birthdayFlag) {
    snprintf(prop, sizeof(prop), "%04d-%02d-%02d", c->birthday.tm_year + 1900,
             c->birthday.tm_mon + 1, c->birthday.tm_mday);
    vcard_emit(f, "BDAY:", prop);
  }

  /* X-PILOT-CATEGORY: non-standard, preserves category on round-trip */
  if (category >= 0 && category < 16 && cai->category.name[category][0])
    vcard_emit(
        f, "X-PILOT-CATEGORY:",
        escape_value(esc_a, sizeof(esc_a), cai->category.name[category]));

  fputs("END:VCARD\r\n", f);
  fputs("\r\n", f); /* blank line between cards */
}

/***********************************************************************
 *
 * Function:	do_write
 *
 * Summary:	Read all records from an open ContactsDB-PAdd and write
 *		each as a vCard 3.0 block to f.
 *
 * Parameters:	f, sd, db, cai
 *
 * Returns:	Number of records written, or -1 on error
 *
 ***********************************************************************/
static int do_write(FILE *f, int sd, int db, struct ContactAppInfo *cai) {
  pi_buffer_t *buf;
  struct Contact c;
  int idx, attr, category, count = 0;
  recordid_t recid;

  buf = pi_buffer_new(0xffff);
  if (buf == NULL) {
    fprintf(stderr, "   ERROR: Memory allocation failed.\n");
    return -1;
  }

  for (idx = 0;; idx++) {
    if (dlp_ReadRecordByIndex(sd, db, idx, buf, &recid, &attr, &category) < 0)
      break;

    if (attr & dlpRecAttrDeleted) continue;
    if (attr & dlpRecAttrArchived) continue;
    if (buf->used == 0) continue;

    if (unpack_Contact(&c, buf, cai->type) < 0) {
      fprintf(stderr,
              "   WARNING: Skipping unreadable record "
              "0x%04x.\n",
              (unsigned int)recid);
      continue;
    }

    write_vcard(f, &c, cai, category);
    free_Contact(&c);
    count++;
  }

  pi_buffer_free(buf);
  return count;
}

/* ------------------------------------------------------------------ */
/* vCard input                                                          */
/* ------------------------------------------------------------------ */

/***********************************************************************
 *
 * Function:	read_logical_line
 *
 * Summary:	Read one logical (unfolded) vCard line from f into buf.
 *		Handles both CRLF and bare-LF line endings.  Folded
 *		continuation lines (CRLF or LF followed immediately by
 *		a space or tab) are appended to the preceding line.
 *
 * Parameters:	f, buf, bufsz
 *
 * Returns:	Number of bytes placed in buf (> 0), 0 on EOF
 *
 ***********************************************************************/
static int read_logical_line(FILE *f, char *buf, size_t bufsz) {
  size_t len = 0;
  int c;

  while (1) {
    c = fgetc(f);
    if (c == EOF) break;

    if (c == '\r') {
      int nx = fgetc(f);
      if (nx != '\n' && nx != EOF) ungetc(nx, f);
      c = '\n';
    }

    if (c == '\n') {
      /* Peek: leading space/tab means folded continuation  */
      int nx = fgetc(f);
      if (nx == ' ' || nx == '\t')
        continue; /* fold: skip whitespace, keep reading */
      if (nx != EOF) ungetc(nx, f);
      break; /* end of this logical line */
    }

    if (len < bufsz - 1) buf[len++] = (char)c;
  }

  buf[len] = '\0';

  if (len == 0 && c == EOF) return 0;

  return (int)len;
}

/***********************************************************************
 *
 * Function:	split_prop
 *
 * Summary:	Split a raw vCard property line into name, params, and
 *		value.  Modifies line in place; on return the three
 *		output pointers reference sub-strings within it.
 *
 *		Example: "TEL;TYPE=WORK,VOICE:+1-555-0100"
 *		  *name   → "TEL"
 *		  *params → "TYPE=WORK,VOICE"
 *		  *value  → "+1-555-0100"
 *
 * Parameters:	line, name, params, value
 *
 * Returns:	0 on success, -1 if the line contains no ':' separator
 *
 ***********************************************************************/
static int split_prop(char *line, char **name, char **params, char **value) {
  char *colon, *semi;

  colon = strchr(line, ':');
  if (colon == NULL) return -1;

  *colon = '\0';
  *value = colon + 1;

  /* Trim leading whitespace from the value.  The vCard spec does
   * not permit a space after ':', but several generators (including
   * Google Contacts and uconv passthrough) emit "BEGIN: VCARD"
   * rather than "BEGIN:VCARD", which would cause every record to
   * be silently skipped.                                           */
  while (**value == ' ' || **value == '\t') (*value)++;

  semi = strchr(line, ';');
  if (semi) {
    *semi = '\0';
    *params = semi + 1;
  } else {
    *params = (char *)"";
  }

  *name = line;
  return 0;
}

/***********************************************************************
 *
 * Function:	type_has_token
 *
 * Summary:	Return 1 if the property parameter string contains the
 *		given TYPE token (case-insensitive).
 *
 *		params may look like "TYPE=WORK,VOICE" or
 *		"TYPE=HOME;TYPE=VOICE" etc.
 *
 * Parameters:	params, token
 *
 * Returns:	1 if found, 0 if not
 *
 ***********************************************************************/
static int type_has_token(const char *params, const char *token) {
  const char *p = params;
  size_t tlen = strlen(token);

  while ((p = ci_strstr(p, "TYPE=")) != NULL) {
    p += 5;
    while (*p) {
      if (strncasecmp(p, token, tlen) == 0) {
        char nx = p[tlen];
        if (nx == '\0' || nx == ',' || nx == ';') return 1;
      }
      while (*p && *p != ',' && *p != ';') p++;
      if (*p == ',') p++;
    }
  }
  return 0;
}

/***********************************************************************
 *
 * Function:	vcard_type_to_phone_label
 *
 * Summary:	Map a vCard TYPE parameter string to a Palm phone label.
 *
 * Parameters:	params — full property parameter string
 *
 * Returns:	Palm phone label 0–7 (default 3 = Other)
 *
 ***********************************************************************/
static int vcard_type_to_phone_label(const char *params) {
  if (type_has_token(params, "CELL") || type_has_token(params, "MOBILE"))
    return 7;
  if (type_has_token(params, "PAGER")) return 6;
  if (type_has_token(params, "FAX")) return 2;
  if (type_has_token(params, "PREF") || type_has_token(params, "MAIN"))
    return 5;
  if (type_has_token(params, "HOME")) return 1;
  if (type_has_token(params, "WORK")) return 0;
  return 3;
}

/***********************************************************************
 *
 * Function:	vcard_type_to_addr_label
 *
 * Summary:	Map a vCard TYPE parameter string to a Palm address label.
 *
 * Parameters:	params
 *
 * Returns:	0=Work, 1=Home, 2=Other
 *
 ***********************************************************************/
static int vcard_type_to_addr_label(const char *params) {
  if (type_has_token(params, "HOME")) return 1;
  if (type_has_token(params, "WORK")) return 0;
  return 2;
}

/***********************************************************************
 *
 * Function:	parse_adr_field
 *
 * Summary:	Extract one semicolon-separated field (0-indexed) from
 *		a vCard ADR value into dst.  Escaped semicolons (\;) are
 *		treated as part of the field value, not as separators.
 *
 *		ADR field layout (RFC 6350):
 *		  0=PO Box  1=Extended  2=Street  3=City
 *		  4=Region  5=Postal    6=Country
 *
 * Parameters:	adr, field, dst, dstsz
 *
 * Returns:	Nothing
 *
 ***********************************************************************/
static void parse_adr_field(const char *adr, int field, char *dst,
                            size_t dstsz) {
  int cur = 0;
  const char *s = adr;
  char *d, *end;

  dst[0] = '\0';

  /* Advance past the fields we're not interested in               */
  while (cur < field) {
    while (*s) {
      if (*s == '\\' && *(s + 1) == ';') {
        s += 2;
        continue;
      }
      if (*s == ';') {
        s++;
        break;
      }
      s++;
    }
    if (*s == '\0') return;
    cur++;
  }

  /* Copy the desired field, unescaping as we go                    */
  d = dst;
  end = dst + dstsz - 1;

  while (*s && d < end) {
    if (*s == '\\' && *(s + 1)) {
      s++;
      switch (*s) {
        case '\\':
          *d++ = '\\';
          break;
        case ',':
          *d++ = ',';
          break;
        case ';':
          *d++ = ';';
          break;
        case 'n': /* fall through */
        case 'N':
          *d++ = '\n';
          break;
        default:
          *d++ = *s;
          break;
      }
      s++;
      continue;
    }
    if (*s == ';') break;
    *d++ = *s++;
  }
  *d = '\0';
}

/***********************************************************************
 *
 * Function:	contact_set_entry
 *
 * Summary:	Set c->entry[idx] to a strdup of val.  Frees any
 *		existing value first.  Skips empty strings.
 *
 * Parameters:	c, idx, val
 *
 * Returns:	0 on success, -1 on allocation failure
 *
 ***********************************************************************/
static int contact_set_entry(struct Contact *c, int idx, const char *val) {
#ifdef HAVE_ICONV
  char conv[VCARD_VAL_MAX];
#endif

  if (val == NULL || val[0] == '\0') return 0;

#ifdef HAVE_ICONV
  /* Convert from UTF-8 (vCard) to the Palm's encoding if needed   */
  if (cd_from_utf8 != (iconv_t)-1) {
    conv_str(cd_from_utf8, val, conv, sizeof(conv));
    val = conv;
  }
#endif

  if (c->entry[idx]) {
    free(c->entry[idx]);
    c->entry[idx] = NULL;
  }

  c->entry[idx] = strdup(val);
  if (c->entry[idx] == NULL) {
    fprintf(stderr, "   ERROR: Memory allocation failed.\n");
    return -1;
  }
  return 0;
}

/***********************************************************************
 *
 * Function:	do_read
 *
 * Summary:	Parse vCard 3.0 records from f and write each to the
 *		open Palm ContactsDB-PAdd.  Records are appended; existing
 *		records on the Palm are not modified.
 *
 * Parameters:	f, sd, db, cai
 *
 * Returns:	Number of records written, or -1 on fatal error
 *
 ***********************************************************************/
static int do_read(FILE *f, int sd, int db, struct ContactAppInfo *cai) {
  char line[VCARD_LINE_MAX];
  char *prop_name, *params, *value;
  struct Contact c;
  pi_buffer_t *buf;
  int in_vcard = 0;
  int phone_slot = 0;
  int addr_slot = 0;
  int im_slot = 0;
  int count = 0;
  char tmp[VCARD_VAL_MAX];
  int n;

  /* IM property name → Palm IM label                              */
  static const struct {
    const char *xprop;
    int label;
  } im_map[] = {{"X-AIM", 1},    {"X-MSN", 2},   {"X-YAHOO", 3}, {"X-ICQ", 4},
                {"X-JABBER", 0}, {"X-GTALK", 0}, {NULL, 0}};

  buf = pi_buffer_new(0xffff);
  if (buf == NULL) {
    fprintf(stderr, "   ERROR: Memory allocation failed.\n");
    return -1;
  }

  while ((n = read_logical_line(f, line, sizeof(line))) > 0) {
    char *p;

    if (split_prop(line, &prop_name, &params, &value) < 0) continue;

    /* Canonicalise property name to uppercase for comparison  */
    for (p = prop_name; *p; p++) *p = (char)toupper((unsigned char)*p);

    /* ---- BEGIN:VCARD ---- */
    if (strcmp(prop_name, "BEGIN") == 0 && strcasecmp(value, "VCARD") == 0) {
      memset(&c, 0, sizeof(c));
      phone_slot = 0;
      addr_slot = 0;
      im_slot = 0;
      in_vcard = 1;
      continue;
    }

    if (!in_vcard) continue;

    /* ---- END:VCARD ---- */
    if (strcmp(prop_name, "END") == 0 && strcasecmp(value, "VCARD") == 0) {
      pi_buffer_clear(buf);
      if (pack_Contact(&c, buf, cai->type) < 0) {
        fprintf(stderr,
                "   WARNING: Could not pack record, "
                "skipping.\n");
      } else if (dlp_WriteRecord(sd, db, 0, 0, 0, buf->data, buf->used, NULL) <
                 0) {
        fprintf(stderr,
                "   WARNING: Could not write record "
                "to Palm.\n");
      } else {
        count++;
        if (!plu_quiet) printf("   Wrote record %d\n", count);
      }
      free_Contact(&c);
      in_vcard = 0;
      continue;
    }

    /* ---- N:Last;First;Middle;Prefix;Suffix ---- */
    if (strcmp(prop_name, "N") == 0) {
      parse_adr_field(value, 0, tmp, sizeof(tmp));
      if (contact_set_entry(&c, contLastname, tmp) < 0) goto error;
      parse_adr_field(value, 1, tmp, sizeof(tmp));
      if (contact_set_entry(&c, contFirstname, tmp) < 0) goto error;
      continue;
    }

    /* ---- FN: — only used if N: gave no name ---- */
    if (strcmp(prop_name, "FN") == 0) {
      if (c.entry[contLastname] == NULL && c.entry[contFirstname] == NULL) {
        unescape_value(value);
        if (contact_set_entry(&c, contFirstname, value) < 0) goto error;
      }
      continue;
    }

    /* ---- ORG: ---- */
    if (strcmp(prop_name, "ORG") == 0) {
      unescape_value(value);
      if (contact_set_entry(&c, contCompany, value) < 0) goto error;
      continue;
    }

    /* ---- TITLE: ---- */
    if (strcmp(prop_name, "TITLE") == 0) {
      unescape_value(value);
      if (contact_set_entry(&c, contTitle, value) < 0) goto error;
      continue;
    }

    /* ---- TEL: ---- */
    if (strcmp(prop_name, "TEL") == 0) {
      if (phone_slot >= 7) continue;
      c.phoneLabel[phone_slot] = vcard_type_to_phone_label(params);
      unescape_value(value);
      if (contact_set_entry(&c, contPhone1 + phone_slot, value) < 0) goto error;
      phone_slot++;
      continue;
    }

    /* ---- EMAIL: ---- */
    if (strcmp(prop_name, "EMAIL") == 0) {
      if (phone_slot >= 7) continue;
      c.phoneLabel[phone_slot] = 4; /* Email */
      unescape_value(value);
      if (contact_set_entry(&c, contPhone1 + phone_slot, value) < 0) goto error;
      /* Show email in list view if it's the only entry  */
      if (phone_slot == 0) c.showPhone = 4;
      phone_slot++;
      continue;
    }

    /* ---- URL: ---- */
    if (strcmp(prop_name, "URL") == 0) {
      unescape_value(value);
      if (contact_set_entry(&c, contWebsite, value) < 0) goto error;
      continue;
    }

    /* ---- NOTE: ---- */
    if (strcmp(prop_name, "NOTE") == 0) {
      unescape_value(value);
      if (contact_set_entry(&c, contNote, value) < 0) goto error;
      continue;
    }

    /* ---- ADR:PO Box;Ext;Street;City;State;ZIP;Country ---- */
    if (strcmp(prop_name, "ADR") == 0) {
      int base;

      if (addr_slot >= 3) continue;

      base = contAddress1 + addr_slot * 5;
      c.addressLabel[addr_slot] = vcard_type_to_addr_label(params);

      parse_adr_field(value, 2, tmp, sizeof(tmp));
      if (contact_set_entry(&c, base, tmp) < 0) goto error;
      parse_adr_field(value, 3, tmp, sizeof(tmp));
      if (contact_set_entry(&c, base + 1, tmp) < 0) goto error;
      parse_adr_field(value, 4, tmp, sizeof(tmp));
      if (contact_set_entry(&c, base + 2, tmp) < 0) goto error;
      parse_adr_field(value, 5, tmp, sizeof(tmp));
      if (contact_set_entry(&c, base + 3, tmp) < 0) goto error;
      parse_adr_field(value, 6, tmp, sizeof(tmp));
      if (contact_set_entry(&c, base + 4, tmp) < 0) goto error;
      addr_slot++;
      continue;
    }

    /* ---- BDAY:YYYY-MM-DD or YYYYMMDD ---- */
    if (strcmp(prop_name, "BDAY") == 0) {
      int yr, mo, da;
      if (sscanf(value, "%4d-%2d-%2d", &yr, &mo, &da) == 3 ||
          sscanf(value, "%4d%2d%2d", &yr, &mo, &da) == 3) {
        memset(&c.birthday, 0, sizeof(c.birthday));
        c.birthday.tm_year = yr - 1900;
        c.birthday.tm_mon = mo - 1;
        c.birthday.tm_mday = da;
        c.birthdayFlag = 1;
      }
      continue;
    }

    /* ---- X-AIM, X-MSN, X-YAHOO, X-ICQ, X-JABBER, X-GTALK ---- */
    if (strncmp(prop_name, "X-", 2) == 0) {
      int xi;
      for (xi = 0; im_map[xi].xprop; xi++)
        if (strcasecmp(prop_name, im_map[xi].xprop) == 0) break;
      if (im_map[xi].xprop != NULL && im_slot < 2) {
        c.IMLabel[im_slot] = im_map[xi].label;
        unescape_value(value);
        if (contact_set_entry(&c, contIM1 + im_slot, value) < 0) goto error;
        im_slot++;
      }
      continue;
    }
  }

  /* Discard any truncated vCard at EOF (no END:VCARD seen)         */
  if (in_vcard) free_Contact(&c);

  pi_buffer_free(buf);
  return count;

error:
  if (in_vcard) free_Contact(&c);
  pi_buffer_free(buf);
  return -1;
}

/* ------------------------------------------------------------------ */
/* Human-readable listing                                               */
/* ------------------------------------------------------------------ */

/***********************************************************************
 *
 * Function:	do_list
 *
 * Summary:	Print a human-readable summary of all Contact records
 *		to stdout.
 *
 * Parameters:	sd, db, cai
 *
 * Returns:	Number of records listed, or -1 on error
 *
 ***********************************************************************/
static int do_list(int sd, int db, struct ContactAppInfo *cai) {
  pi_buffer_t *buf;
  struct Contact c;
  int idx, attr, category, count = 0;
  recordid_t recid;

  static const char *phone_names[] = {"Work",  "Home", "Fax",   "Other",
                                      "Email", "Main", "Pager", "Mobile"};
  static const char *im_names[] = {"IM", "AIM", "MSN", "Yahoo", "ICQ"};
  static const char *addr_names[] = {"Work", "Home", "Other"};

  buf = pi_buffer_new(0xffff);
  if (buf == NULL) {
    fprintf(stderr, "   ERROR: Memory allocation failed.\n");
    return -1;
  }

  for (idx = 0;; idx++) {
    int k;

    if (dlp_ReadRecordByIndex(sd, db, idx, buf, &recid, &attr, &category) < 0)
      break;

    if (attr & dlpRecAttrDeleted) continue;
    if (attr & dlpRecAttrArchived) continue;
    if (buf->used == 0) continue;

    if (unpack_Contact(&c, buf, cai->type) < 0) {
      fprintf(stderr,
              "   WARNING: Skipping unreadable record "
              "0x%04x.\n",
              (unsigned int)recid);
      continue;
    }

    printf("\n--- Record 0x%04x  Category: %s ---\n", (unsigned int)recid,
           cai->category.name[category]);

    if (c.entry[contFirstname] || c.entry[contLastname])
      printf("  Name    : %s%s%s\n",
             c.entry[contFirstname] ? c.entry[contFirstname] : "",
             (c.entry[contFirstname] && c.entry[contLastname]) ? " " : "",
             c.entry[contLastname] ? c.entry[contLastname] : "");

    if (c.entry[contCompany]) printf("  Company : %s\n", c.entry[contCompany]);
    if (c.entry[contTitle]) printf("  Title   : %s\n", c.entry[contTitle]);

    for (k = 0; k < 7; k++) {
      int lbl = c.phoneLabel[k];
      if (c.entry[contPhone1 + k]) {
        const char *ln = (lbl >= 0 && lbl < 8) ? phone_names[lbl] : "Phone";
        printf("  %-8s: %s\n", ln, c.entry[contPhone1 + k]);
      }
    }

    for (k = 0; k < 2; k++) {
      int lbl = c.IMLabel[k];
      if (c.entry[contIM1 + k]) {
        const char *ln = (lbl >= 0 && lbl < 5) ? im_names[lbl] : "IM";
        printf("  %-8s: %s\n", ln, c.entry[contIM1 + k]);
      }
    }

    if (c.entry[contWebsite]) printf("  Website : %s\n", c.entry[contWebsite]);

    for (k = 0; k < 3; k++) {
      int base = addr_base[k];
      int albl = c.addressLabel[k];
      const char *aname = (albl >= 0 && albl < 3) ? addr_names[albl] : "Addr";

      if (!c.entry[base]) continue;

      printf("  %s addr:\n", aname);
      if (c.entry[base]) printf("    %s\n", c.entry[base]);
      if (c.entry[base + 1] || c.entry[base + 2] || c.entry[base + 3])
        printf("    %s%s%s %s\n", c.entry[base + 1] ? c.entry[base + 1] : "",
               (c.entry[base + 1] && c.entry[base + 2]) ? ", " : "",
               c.entry[base + 2] ? c.entry[base + 2] : "",
               c.entry[base + 3] ? c.entry[base + 3] : "");
      if (c.entry[base + 4]) printf("    %s\n", c.entry[base + 4]);
    }

    if (c.entry[contNote]) printf("  Note    : %s\n", c.entry[contNote]);

    if (c.birthdayFlag)
      printf("  Birthday: %04d-%02d-%02d\n", c.birthday.tm_year + 1900,
             c.birthday.tm_mon + 1, c.birthday.tm_mday);

    if (c.picture) printf("  Picture : JPEG (%u bytes)\n", c.picture->length);

    free_Contact(&c);
    count++;
  }

  pi_buffer_free(buf);
  return count;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(int argc, const char *argv[]) {
  int c;
  int sd = -1;
  int db;
  int run_mode = mode_none;
  int delete_all = 0;
  int count = 0;
  int db_flags;
  char *wrFilename = NULL;
  char *rdFilename = NULL;
  const char *charset = NULL;
  struct PilotUser User;
  struct ContactAppInfo cai;
  pi_buffer_t *appblock;
  poptContext po;

  struct poptOption options[] = {
      USERLAND_RESERVED_OPTIONS{
          "write", 'w', POPT_ARG_STRING, &wrFilename, 'w',
          "Read contacts from Palm, write vCards to <file>", "file"},
      {"read", 'r', POPT_ARG_STRING, &rdFilename, 'r',
       "Read vCards from <file>, install contacts to Palm", "file"},
      {"list", 'l', POPT_ARG_NONE, NULL, 'l',
       "List contacts on Palm (human-readable)", NULL},
      {"delete-all", 'D', POPT_ARG_NONE, &delete_all, 0,
       "Delete all contacts on Palm before any other action", NULL},
#ifdef HAVE_ICONV
      {"charset", 'e', POPT_ARG_STRING, &charset, 0,
       "Palm character encoding for non-UTF-8 devices "
       "(e.g. GB2312, BIG5, CP1252)",
       "enc"},
#endif
      POPT_TABLEEND};

  po = poptGetContext("pilot-contacts", argc, argv, options, 0);
  poptSetOtherOptionHelp(
      po,
      "\n\n"
      "   Read or write Palm OS 5 Contacts (ContactsDB-PAdd)\n"
      "   using vCard 3.0 format for import and export.\n\n"
      "   Provide exactly one of --read, --write, or --list.\n\n");

  if (argc < 2) {
    poptPrintUsage(po, stderr, 0);
    return 1;
  }

  while ((c = poptGetNextOpt(po)) >= 0) {
    if (run_mode != mode_none && c != 0) {
      fprintf(stderr,
              "   ERROR: Specify exactly one of "
              "--read, --write, or --list.\n");
      return 1;
    }
    switch (c) {
      case 'w':
        run_mode = mode_write;
        break;
      case 'r':
        run_mode = mode_read;
        break;
      case 'l':
        run_mode = mode_list;
        break;
      default:
        fprintf(stderr, "   ERROR: Unhandled option %d.\n", c);
        return 1;
    }
  }
  if (c < -1) plu_badoption(po, c);

  if (run_mode == mode_none && !delete_all) {
    fprintf(stderr,
            "   ERROR: Specify at least one of "
            "--read, --write, --list, or --delete-all.\n");
    return 1;
  }

  if (delete_all && (run_mode == mode_write || run_mode == mode_list)) {
    fprintf(stderr,
            "   ERROR: --delete-all cannot be combined "
            "with --%s.\n",
            run_mode == mode_write ? "write" : "list");
    return 1;
  }

#ifdef HAVE_ICONV
  /* Open charset conversion descriptors if --charset was given and
   * the encoding is not already UTF-8.                             */
  if (charset != NULL && strcasecmp(charset, "UTF-8") != 0 &&
      strcasecmp(charset, "UTF8") != 0) {
    cd_to_utf8 = iconv_open("UTF-8", charset);
    if (cd_to_utf8 == (iconv_t)-1) {
      fprintf(stderr,
              "   ERROR: Cannot convert from '%s' to "
              "UTF-8: %s\n",
              charset, strerror(errno));
      return 1;
    }
    cd_from_utf8 = iconv_open(charset, "UTF-8");
    if (cd_from_utf8 == (iconv_t)-1) {
      fprintf(stderr,
              "   ERROR: Cannot convert from UTF-8 to "
              "'%s': %s\n",
              charset, strerror(errno));
      iconv_close(cd_to_utf8);
      return 1;
    }
    if (!plu_quiet) printf("   Charset: %s\n", charset);
  }
#endif

  sd = plu_connect();
  if (sd < 0) goto error;

  if (dlp_ReadUserInfo(sd, &User) < 0) goto error_close;

  /* Open the database read-only for list/write; read-write for
   * read or delete operations.                                      */
  db_flags = (run_mode == mode_read || delete_all)
                 ? (dlpOpenRead | dlpOpenWrite)
                 : dlpOpenRead;

  if (dlp_OpenDB(sd, 0, db_flags, "ContactsDB-PAdd", &db) < 0) {
    fprintf(stderr, "   ERROR: Unable to open ContactsDB-PAdd on Palm.\n");
    dlp_AddSyncLogEntry(sd,
                        "pilot-contacts: Unable to open ContactsDB-PAdd.\n");
    goto error_close;
  }

  appblock = pi_buffer_new(0xffff);
  if (appblock == NULL) {
    fprintf(stderr, "   ERROR: Memory allocation failed.\n");
    goto error_close;
  }
  if (dlp_ReadAppBlock(sd, db, 0, -1, appblock) < 0) {
    fprintf(stderr, "   ERROR: Unable to read app info block.\n");
    pi_buffer_free(appblock);
    goto error_close;
  }
  if (unpack_ContactAppInfo(&cai, appblock) < 0) {
    fprintf(stderr, "   ERROR: Unable to unpack app info block.\n");
    pi_buffer_free(appblock);
    goto error_close;
  }
  pi_buffer_free(appblock);

  if (delete_all) {
    if (!plu_quiet) printf("   Deleting all contacts on Palm...\n");
    if (dlp_DeleteRecord(sd, db, 1, 0) < 0) {
      fprintf(stderr, "   ERROR: Failed to delete records.\n");
      goto error_close;
    }
    if (!plu_quiet) printf("   All contacts deleted.\n");
  }

  switch (run_mode) {
    case mode_none:
      /* --delete-all was the only action; nothing more to do    */
      break;

    case mode_write: {
      FILE *f;
      int old_quiet = plu_quiet;

      if (strcmp(wrFilename, "-") == 0) {
        f = stdout;
        plu_quiet = 1;
      } else {
        f = fopen(wrFilename, "w");
      }
      if (f == NULL) {
        fprintf(stderr, "   ERROR: Cannot open '%s': %s\n", wrFilename,
                strerror(errno));
        goto error_close;
      }
      count = do_write(f, sd, db, &cai);
      if (f == stdout)
        plu_quiet = old_quiet;
      else
        fclose(f);
      if (!plu_quiet && count >= 0)
        printf("\n   Wrote %d contact(s) to %s.\n", count, wrFilename);
      break;
    }

    case mode_read: {
      FILE *f = fopen(rdFilename, "r");
      if (f == NULL) {
        fprintf(stderr, "   ERROR: Cannot open '%s': %s\n", rdFilename,
                strerror(errno));
        goto error_close;
      }
      /* Strip UTF-8 BOM (0xEF 0xBB 0xBF) if present.
       * Tools like uconv write it by default; without
       * stripping it the first property name becomes
       * "\xEF\xBB\xBFBEGIN" and the parser finds nothing. */
      {
        unsigned char bom[3];
        if (fread(bom, 1, 3, f) == 3 && bom[0] == 0xEF && bom[1] == 0xBB &&
            bom[2] == 0xBF) {
          if (!plu_quiet) printf("   (Skipping UTF-8 BOM)\n");
        } else {
          rewind(f);
        }
      }
      count = do_read(f, sd, db, &cai);
      fclose(f);
      if (!plu_quiet && count >= 0)
        printf("\n   Installed %d contact(s) to Palm.\n", count);
      break;
    }

    case mode_list:
      count = do_list(sd, db, &cai);
      if (!plu_quiet && count >= 0)
        printf("\n   %d contact(s) listed.\n", count);
      break;

    default:
      break;
  }

  free_ContactAppInfo(&cai);
  dlp_CloseDB(sd, db);

  if (delete_all && run_mode == mode_read)
    dlp_AddSyncLogEntry(sd,
                        "pilot-contacts: Replaced all contacts on Palm.\n"
                        "Thank you for using pilot-link.");
  else if (delete_all)
    dlp_AddSyncLogEntry(sd,
                        "pilot-contacts: Deleted all contacts on Palm.\n"
                        "Thank you for using pilot-link.");
  else if (run_mode == mode_read)
    dlp_AddSyncLogEntry(sd,
                        "pilot-contacts: Installed contacts to Palm.\n"
                        "Thank you for using pilot-link.");
  else
    dlp_AddSyncLogEntry(sd,
                        "pilot-contacts: Read contacts from Palm.\n"
                        "Thank you for using pilot-link.");

  dlp_EndOfSync(sd, 0);
  pi_close(sd);
  poptFreeContext(po);
#ifdef HAVE_ICONV
  if (cd_to_utf8 != (iconv_t)-1) iconv_close(cd_to_utf8);
  if (cd_from_utf8 != (iconv_t)-1) iconv_close(cd_from_utf8);
#endif
  return (count >= 0) ? 0 : 1;

error_close:
  pi_close(sd);
error:
  poptFreeContext(po);
#ifdef HAVE_ICONV
  if (cd_to_utf8 != (iconv_t)-1) iconv_close(cd_to_utf8);
  if (cd_from_utf8 != (iconv_t)-1) iconv_close(cd_from_utf8);
#endif
  return 1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
