#ifndef _JSTR_H_
#define _JSTR_H_

/* jstrcnv */
#define JTOPUNS  (0x0001)
#define JTODGT   (0x0002)
#define JTOALPH  (0x0004)
#define JTOKPUN  (0x0008)
#define JTOKATA  (0x0010)
#define JTOHIRA  (0x0020)
#define JSPC2SPC (0x0040)
#define YENOFF   (0x0080)
#define TOJPUNS  (0x4100)
#define TOJPUN   (0x0100)
#define TOJDGT   (0x0200)
#define TOJALPH  (0x0400)
#define TOJKPUN  (0x0800)
#define TOJKATA  (0x1000)
#define TOJHIRA  (0x2000)
#define TOJSPC   (0x4000)
#define DAKUOFF  (0x8000)

/* jstruplow */
#define A2UPPER  (0x0001)
#define A2LOWER  (0x0002)
#define J2UPPER  (0x0004)
#define J2LOWER  (0x0008)
#define J2KATA   (0x0010)
#define J2HIRA   (0x0020)
/*
#define JSPC2SPC (0x0040)
*/
char *jstrcnv(char *, char const* ,unsigned);
char *jstruplow(char *, unsigned);

#endif
