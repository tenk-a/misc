/*
 *	@file	tee.c
 *	@brief	tiny tee
 */
#include <stdio.h>

int main(int argc, char* argv[])
{
	FILE* fp;
	static char buf[0x8000];
	char const* mode = "wb";
	char* fname = NULL;

	if (argc < 2) {
		fputs("tiny-tee: (build:" __DATE__ ")\n"
			  "usage> tee [-a] outputfile\n"
			 , stderr
		);
		return 1;
	}
	fname = argv[1];
	if (argc >= 3 && fname[0] == '-' && fname[1] == 'a') {
		mode = "ab";
		fname = argv[2];
	}
	fp = fopen(fname, mode);
	if (!fp) {
		fputs(fname, stderr);
		fputs(" : File open error\n", stderr);
		return 1;
	}
	while (fgets(buf, sizeof buf, stdin) != NULL) {
		fputs(buf, fp);
		fputs(buf, stdout);
	}
	fclose(fp);
	return 0;
}
