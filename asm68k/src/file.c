/*
 	include 命令 対策
 
 	include 用 ファイルopen,fgets,close
 */
#include	"asm.h"
#include	<stdarg.h>

#define IX_MAX 17
int  IX_no = 0;
static struct IX_T {
	FILE *fp;
	char *name;
	int  line;
} IX_tbl[IX_MAX], *ix;

void IX_Error(const char *fmt, ...)
{
	char buf[260];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	fprintf(STDERR, "%-16s %5d : ", ix->name, ix->line);
	fprintf(STDERR, buf);
	return;
}

int IX_Open(char *name)
{
	if (IX_no >= IX_MAX) {
		IX_Error("include nest is deep!\n");
	}
	if (IX_no == 0) {
		memset(IX_tbl,0,sizeof IX_tbl);
		ix = &IX_tbl[IX_no];
	}
	IX_no++;
	ix = &IX_tbl[IX_no];
	lineNum  = ix->line = 0;
	fileName = ix->name = strdup(name);
	ix->fp = fopen(ix->name, "rt");
	if (ix->fp == NULL) {
		fprintf(STDERR,"file'%s' not found\n", name);
		exit(1);
	}
//IX_Error("open @%d %08x\n", IX_no, ix->fp);
	return ferror(ix->fp);
}

int IX_Close(void)
{
	//IX_Error("close @%d %08x\n", IX_no, ix->fp);
	if (ix->fp)
		fclose(ix->fp);
	ix->fp   = NULL;
	if (ix->name)
		free(ix->name);
	ix->name = NULL;
	ix->line = 0;

	if (IX_no) {
		--IX_no;
		ix = &IX_tbl[IX_no];
		fileName = ix->name;
		lineNum = ix->line;
	}
	return 0;
}

static char *skipspc(char *p)
{
	if (p) {
		while (*p && *(unsigned char *)p <= ' ')
			p++;
	}
	return p;
}

char *IX_Gets(char *buf, int sz)
{
	char *s;

	if (ix == NULL || ix->fp == NULL || buf == NULL) {
		IX_Error("PRGERR: ix(%x),fp(%x) or buf(%x) is NULL\n", ix,ix->fp,buf);
		return NULL;
	}
	s = fgets(buf,sz,ix->fp);
	ix->line++;
	lineNum = ix->line;
	fileName = ix->name;
//IX_Error("[%d] %s", IX_no, s);
	return s;
}

char *IX_GetsInc(char *buf, int sz)
{
	char *s,*p;

  J1:
	s = IX_Gets(buf,sz);
	if (s) {
//printf("@\n");
		p = skipspc(s);
//printf("@\n");
		if (p && strnicmp(p,"INCLUDE",7) == 0) {
			p+=7;
			if (*p && *(unsigned char *)p <= ' ') {
				p = strtok(p," '\t\"\n");
				if (p == NULL) {
					IX_Error("bad include\n");
					exit(1);
				}
				IX_Open(p);
				goto J1;
			}
		}
	} else {
		IX_Close();
		if (IX_no && ix->fp) {
//IX_Error("k[%d]\n", IX_no, s);
			goto J1;
		}
//IX_Error("m[%d]\n", IX_no, s);
		s = NULL;
	}
	return s;
}

void IX_Rewind(void)
{
	if (ix->fp)
		rewind(ix->fp);
}
