/*
 *	@file	tee.c
 *	@brief	tiny tee
 */
#include <stdio.h>

int main(int argc, char* argv[])
{
	char buf[0x8000] = {0};
	FILE* fp;
	if (argc != 2) {
		fputs("tiny-tee: build " __DATE__ "\n"
			  "usage> tee outputfile\n"
			 , stderr
		);
		return 1;
	}
	fp = fopen(argv[1], "wb");
	if (!fp) {
		fputs(argv[1], stderr);
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
