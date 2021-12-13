/*
 *  @file   tee.c
 *  @brief  tee
 *  @author Masashi Kitamura (tenka@6809.net)
 *  @date   2021-11
 *  note
 *    Boost Software License Version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

int usage() {
    fputs("usage> tee [-a] outputfile(s)\n"
          "  -a   Append mode.\n"
         , stderr
    );
    return 1;
}

int main(int argc, char* argv[]) {
    static char buf[0x8000];
    if (argc < 2)
        return usage();
    char const* mode = "w";
    size_t count = 0;
    char   flag = 0;
    for (size_t i = 1; i < (size_t)argc; ++i) {
        char* p = argv[i];
        if (*p == '-' && !flag) {
            ++p;
            if (*p == 'a')
                mode = "a";
            else if (*p == '-')
                flag = 1;
            else
                return usage();
        } else {
            ++count;
        }
    }
    if (!count)
        return usage();
    FILE* files[count];
    size_t j = 0;
    for (size_t i = 1; i < (size_t)argc; ++i) {
        char* p = argv[i];
        if (*p == '-' && !flag) {
            if (p[1] == '-')
                flag = 1;
        } else {
            files[j] = fopen(p, mode);
            if (!files[j]) {
                fputs(p, stderr);
                fputs(" : File open error\n", stderr);
                return 1;
            }
            ++j;
        }
    }
    while (fgets(buf, sizeof buf, stdin) != NULL) {
        fputs(buf, stdout);
        for (size_t i = 0; i < count; ++i)
            fputs(buf, files[i]);
    }
    for (size_t i = 0; i < count; ++i)
        fclose(files[i]);
    return 0;
}
