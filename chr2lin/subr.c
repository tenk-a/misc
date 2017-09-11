#include "subr.h"
#include <stdarg.h>
#include <io.h>

int		debugflag;

char *StrNCpyZ(char *dst, char *src, size_t size)
{
	strncpy(dst, src, size);
	dst[size-1] = 0;
	return dst;
}

char *StrSkipSpc(char *s)
{
	while ((*s && *(unsigned char *)s <= ' ') || *s == 0x7f) {
		s++;
	}
	return s;
}

char *StrDelLf(char *s)
{
	char *p;
	p = STREND(s);
	if (p != s && p[-1] == '\n') {
		p[-1] = 0;
	}
	return s;
}

char *FIL_BaseName(char *adr)
{
	char *p;

	p = adr;
	while (*p != '\0') {
		if (*p == ':' || *p == '/' || *p == '\\')
			adr = p + 1;
		if (ISKANJI((*(unsigned char *)p)) && *(p+1) )
			p++;
		p++;
	}
	return adr;
}

char *FIL_ExtPtr(char *name)
{
	char *p;

	name = FIL_BaseName(name);
	p = strrchr(name, '.');
	if (p) {
		return p+1;
	}
	return STREND(name);
}


char *FIL_ChgExt(char filename[], char *ext)
{
	char *p;

	p = FIL_BaseName(filename);
	p = strrchr( p, '.');
	if (p == NULL) {
		if (ext) {
			strcat(filename,".");
			strcat( filename, ext);
		}
	} else {
		if (ext == NULL)
			*p = 0;
		else
			strcpy(p+1, ext);
	}
	return filename;
}


char *FIL_AddExt(char filename[], char *ext)
{
	if (strrchr(FIL_BaseName(filename), '.') == NULL) {
		strcat(filename,".");
		strcat(filename, ext);
	}
	return filename;
}

char *FIL_DelLastDirSep(char *dir)
{
	char *p;

	if (dir) {
		p = FIL_BaseName(dir);
		if (strlen(p) > 1) {
			p = STREND(dir);
			if (p[-1] == '\\' || p[-1] == '/')
				p[-1] = 0;
		}
	}
	return dir;
}

char *FIL_DirNameChgExt(char *nam, char *dir, char *name, char *chgext)
{
	if (name == NULL || strcmp(name,".") == 0)
		return NULL;
	if (dir && *dir) {
		sprintf(nam, "%s\\%s", dir, name);
	} else {
		sprintf(nam, "%s", name);
	}
	FIL_ChgExt(nam, chgext);
	strupr(nam);
	return nam;
}

char *FIL_DirNameAddExt(char *nam, char *dir, char *name, char *addext)
{
	if (name == NULL || strcmp(name,".") == 0)
		return NULL;
	if (dir && *dir) {
		sprintf(nam, "%s\\%s", dir, name);
	} else {
		sprintf(nam, "%s", name);
	}
	FIL_AddExt(nam, addext);
	strupr(nam);
	return nam;
}


/*--------------------- �G���[�����t���̕W���֐� ---------------------------*/
volatile void err_exit(char *fmt, ...)
{
	va_list app;

	va_start(app, fmt);
/*	fprintf(stdout, "%s %5d : ", src_name, src_line);*/
	vfprintf(stdout, fmt, app);
	va_end(app);
	exit(1);
}

void *mallocE(size_t a)
	/* �G���[������Α�exit�� malloc() */
{
	void *p;
#if 1
	if (a == 0)
		a = 1;
#endif
	p = malloc(a);
//printf("malloc(0x%x)\n",a);
	if (p == NULL) {
		err_exit("������������Ȃ�(%d byte(s))\n",a);
	}
	return p;
}

void *callocE(size_t a, size_t b)
	/* �G���[������Α�exit�� calloc() */
{
	void *p;

#if 1
	if (a== 0 || b == 0)
		a = b = 1;
#endif
	p = calloc(a,b);
//printf("calloc(0x%x,0x%x)\n",a,b);
	if (p == NULL) {
		err_exit("������������Ȃ�(%d*%d byte(s))\n",a,b);
	}
	return p;
}

void *reallocE(void *m, size_t a)
	/* �G���[������Α�exit�� calloc() */
{
	void *p;
	if (a == 0)
		a = 1;
	p = realloc(m, a);
//printf("realloc(0x%x,0x%x)\n",m,a);
	if (p == NULL) {
		err_exit("������������Ȃ�(%d byte(s))\n",a);
	}
	return p;
}

char *strdupE(char *s)
	/* �G���[������Α�exit�� strdup() */
{
	char *p;
//printf("strdup('%s')\n",s);
	if (s == NULL)
		return callocE(1,1);
  #if 1
	p = calloc(1,strlen(s)+8);
	if (p)
		strcpy(p, s);
  #else
	p = strdup(s);
  #endif
	if (p == NULL) {
		err_exit("������������Ȃ�(����%d+1)\n",strlen(s));
	}
	return p;
}

int freeE(void *p)
{
	if (p)
		free(p);
	return 0;
}

/* ------------------------------------------------------------------------ */
FILE *fopenE(char *name, char *mod)
	/* �G���[������Α�exit�� fopen() */
{
	FILE *fp;
	fp = fopen(name,mod);
	if (fp == NULL) {
		err_exit("�t�@�C�� %s ���I�[�v���ł��܂���\n",name);
	}
	setvbuf(fp, NULL, _IOFBF, 1024*1024);
	return fp;
}

size_t  fwriteE(void *buf, size_t sz, size_t num, FILE *fp)
	/* �G���[������Α�exit�� fwrite() */
{
	size_t l;

	l = fwrite(buf, sz, num, fp);
	if (ferror(fp)) {
		err_exit("�t�@�C�������݂ŃG���[����\n");
	}
	return l;
}

size_t  freadE(void *buf, size_t sz, size_t num, FILE *fp)
	/* �G���[������Α�exit�� fread() */
{
	size_t l;

	l = fread(buf, sz, num, fp);
	if (ferror(fp)) {
		err_exit("�t�@�C���Ǎ��݂ŃG���[����\n");
	}
	return l;
}

/* ------------------------------------------------------------------------ */

int fgetcE(FILE *fp)
	/* fp��� 1�o�C�g(0..255) �ǂݍ���. �G���[������Α��I�� */
{
  #if 1
	static UCHAR buf[1];
	freadE(buf, 1, 1, fp);
	return (UCHAR)buf[0];
  #else
	int c;
	c = fgetc(fp);
	if (c == EOF) {
		err_exit("�t�@�C���ǂݍ��݂ŃG���[����\n");
	}
	return (UCHAR)c;
  #endif
}

int fgetc2iE(FILE *fp)
	/* fp��� ��ٴ��ި�݂� 2�o�C�g�ǂݍ���. �G���[������Α��I�� */
{
	int c;
	c = fgetcE(fp);
	return (USHORT)(c + (fgetcE(fp)<<8));
}

int fgetc4iE(FILE *fp)
	/* fp��� ��ٴ��ި�݂� 4�o�C�g�ǂݍ���. �G���[������Α��I�� */
{
	int c;
	c = fgetc2iE(fp);
	return c + (fgetc2iE(fp)<<16);
}

int fgetc2mE(FILE *fp)
	/* fp��� big���ި�݂� 2�o�C�g�ǂݍ���. �G���[������Α��I�� */
{
	int c;
	c = fgetcE(fp);
	return (USHORT)((c<<8) + fgetcE(fp));
}

int fgetc4mE(FILE *fp)
	/* fp��� big���ި�݂� 4�o�C�g�ǂݍ���. �G���[������Α��I�� */
{
	int c;
	c = fgetc2mE(fp);
	return (c<<16) + fgetc2mE(fp);
}

void fputcE(int c, FILE *fp)
	/* fp�� 1�o�C�g(0..255) ��������. �G���[������Α��I�� */
{
	static UCHAR buf[1];
	buf[0] = (UCHAR)c;
	fwriteE(buf, 1, 1, fp);
}

void fputc2mE(int c, FILE *fp)
	/* fp�� �ޯ�޴��ި�݂� 2�o�C�g��������. �G���[������Α��I�� */
{
	static UCHAR buf[4];
	buf[0] = (UCHAR)(c>> 8);
	buf[1] = (UCHAR)(c);
	fwriteE(buf, 1, 2, fp);
}

void fputc4mE(int c, FILE *fp)
	/* fp�� �ޯ�޴��ި�݂� 4�o�C�g��������. �G���[������Α��I�� */
{
	static UCHAR buf[4];
	buf[0] = (UCHAR)(c>>24);
	buf[1] = (UCHAR)(c>>16);
	buf[2] = (UCHAR)(c>> 8);
	buf[3] = (UCHAR)(c);
	fwriteE(buf, 1, 4, fp);
}

void *fputsE(char *s, FILE *fp)
{
	int n;
	n = strlen(s);
	fwriteE(s, 1, n, fp);
	return s;
}

void fputc2iE(int c, FILE *fp)
	/* fp�� ��ٴ��ި�݂� 2�o�C�g��������. �G���[������Α��I�� */
{
	static UCHAR buf[4];
	buf[0] = (UCHAR)(c);
	buf[1] = (UCHAR)(c>> 8);
	fwriteE(buf, 1, 2, fp);
}

void fputc4iE(int c, FILE *fp)
	/* fp�� ��ٴ��ި�݂� 4�o�C�g��������. �G���[������Α��I�� */
{
	static UCHAR buf[4];
	buf[0] = (UCHAR)(c);
	buf[1] = (UCHAR)(c>> 8);
	buf[2] = (UCHAR)(c>> 16);
	buf[3] = (UCHAR)(c>> 24);
	fwriteE(buf, 1, 4, fp);
}

/* ------------------------------------------------------------------------ */
int FIL_GetTmpDir(char *t)
{
	char *p;
	char nm[FIL_NMSZ+2];

	if (*t) {
		strcpy(nm, t);
		p = STREND(nm);
	} else {
		p = getenv("TMP");
		if (p == NULL) {
			p = getenv("TEMP");
			if (p == NULL) {
			  #if 0
				p = ".\\";
			  #else
				err_exit("���ϐ�TMP��TEMP�������إ�ިڸ�؂��w�肵�Ă�������\n");
			  #endif
			}
		}
		strcpy(nm, p);
		p = STREND(nm);
	}
	if (p[-1] != '\\' && p[-1] != ':' && p[-1] != '/')
		strcat(nm,"\\");
	strcat(nm,"*.*");
	_fullpath(t, nm, FIL_NMSZ);
	p = FIL_BaseName(t);
	*p = 0;
	if (p[-1] == '\\')
		p[-1] = 0;
	return 0;
}

#if 0
char *FIL_DirNameAddExtM(char *dir, char *name, char *addext)
{
	char *p;

	if (name == NULL || strcmp(name,".") == 0)
		return NULL;
	if (dir && *dir) {
		p = mallocE(strlen(dir) + strlen(name) + (32));
		sprintf(p, "%s\\%s", dir, name);
	} else {
		p = mallocE(strlen(name) + (32));
		sprintf(p, "%s", name);
	}
	FIL_AddExt(p, addext);
	return p;
}
#endif

void FIL_SaveE(char *name, void *buf, int size)
{
	FILE *fp;

	fp = fopenE(name,"wb");
	if (size)
		fwriteE(buf, 1, size, fp);
	fclose(fp);
}

void FIL_LoadE(char *name, void *buf, int size)
{
	FILE *fp;

	if (size) {
		fp = fopenE(name,"rb");
		if (size)
			freadE(buf, 1, size, fp);
		fclose(fp);
	}
}

int		FIL_loadsize;

void *FIL_LoadME(char *name)
{
	FILE *fp;
	char  *buf;
	int   l;

	fp = fopenE(name,"rb");
	l  = filelength(fileno(fp));
	FIL_loadsize = l;
	if (l) {
		l = (l + 15) & ~15;
		buf = callocE(1,l);
		freadE(buf, 1, l, fp);
	} else {
		buf = mallocE(16);
	}
	fclose(fp);
	return buf;
}

void *FIL_LoadM(char *name)
{
	FILE *fp;
	char  *buf;
	int   l;

	fp = fopen(name,"rb");
	if (fp == NULL)
		return NULL;
	l  = filelength(fileno(fp));
	FIL_loadsize = l;
	if (l) {
		l = (l + 15) & ~15;
		buf = calloc(1,l);
		if (buf)
			freadE(buf, 1, l, fp);
	} else {
		buf = NULL/*mallocE(16)*/;
	}
	fclose(fp);
	return buf;
}


/* ------------------------------------------------------------------------ */
ULONG	TXT1_line;
char	TXT1_name[FIL_NMSZ];
FILE	*TXT1_fp;

void TXT1_Error(char *fmt, ...)
{
	va_list app;

	va_start(app, fmt);
	fprintf(stdout, "%-12s %5d : ", TXT1_name, TXT1_line);
	vfprintf(stdout, fmt, app);
	va_end(app);
	return;
}

void TXT1_ErrorE(char *fmt, ...)
{
	va_list app;

	va_start(app, fmt);
	fprintf(stdout, "%-12s %5d : ", TXT1_name, TXT1_line);
	vfprintf(stdout, fmt, app);
	va_end(app);
	exit(1);
}

int TXT1_Open(char *name)
{
	TXT1_fp = fopen(name,"rt");
	if (TXT1_fp == 0)
		return -1;
	strcpy(TXT1_name, name);
	TXT1_line = 0;
	return 0;
}

void TXT1_OpenE(char *name)
{
	TXT1_fp = fopenE(name,"rt");
	strcpy(TXT1_name, name);
	TXT1_line = 0;
}


char *TXT1_GetsE(char *buf, int sz)
{
	char *p;

	p = fgets(buf, sz, TXT1_fp);
	if (ferror(TXT1_fp)) {
		TXT1_Error("file read error\n");
		exit(1);
	}
	TXT1_line++;
	return p;
}

void TXT1_Close(void)
{
	fclose(TXT1_fp);
}


/* ------------------------------------------------------------------------ */

SLIST *SLIST_Add(SLIST **p0, char *s)
{
	SLIST* p;
	p = *p0;
	if (p == NULL) {
		p = callocE(1, sizeof(SLIST));
		p->s = strdupE(s);
		*p0 = p;
	} else {
		while (p->link != NULL) {
			p = p->link;
		}
		p->link = callocE(1, sizeof(SLIST));
		p = p->link;
		p->s = strdupE(s);
	}
	return p;
}

void SLIST_Free(SLIST **p0)
{
	SLIST *p, *q;

	for (p = *p0; p; p = q) {
		q = p->link;
		freeE(p->s);
		freeE(p);
	}
}

/* ------------------------------------------------------------------------ */
static STBL_CMP STBL_cmp = (STBL_CMP)strcmp;

STBL_CMP STBL_SetFncCmp(STBL_CMP cmp)
{
	if (cmp)
		STBL_cmp = cmp;
	return STBL_cmp;
}

int STBL_Add(void *t[], int *tblcnt, void *key)
   /*
	*  t     : ������ւ̃|�C���^�������߂��z��
	*  tblcnt: �o�^�ό�
	*  key   : �ǉ����镶����
	*  ���A�l: 0:�ǉ� -1:���łɓo�^��
	*/
{
	int  low, mid, f, hi;

	hi = *tblcnt;
	mid = low = 0;
	while (low < hi) {
		mid = (low + hi - 1) / 2;
		if ((f = STBL_cmp(key, t[mid])) < 0) {
			hi = mid;
		} else if (f > 0) {
			mid++;
			low = mid;
		} else {
			return -1;	/* �������̂��݂������̂Œǉ����Ȃ� */
		}
	}
	(*tblcnt)++;
	for (hi = *tblcnt; --hi > mid;) {
		t[hi] = t[hi-1];
	}
	t[mid] = key;
	return 0;
}

int STBL_Search(void *tbl[], int nn, void *key)
   /*
	*  key:������������ւ̃|�C���^
	*  tbl:������ւ̃|�C���^�������߂��z��
	*  nn:�z��̃T�C�Y
	*  ���A�l:��������������̔ԍ�(0���)  �݂���Ȃ������Ƃ�-1
	*/
{
	int     low, mid, f;

	low = 0;
	while (low < nn) {
		mid = (low + nn - 1) / 2;
		if ((f = STBL_cmp(key, tbl[mid])) < 0)
			nn = mid;
		else if (f > 0)
			low = mid + 1;
		else
			return mid;
	}
	return -1;
}
