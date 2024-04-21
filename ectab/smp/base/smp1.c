#include <stdio.h>
#include "subr.h"

int main(int argc, char* argv[]) {
    // 文字列のうしろに空白.        
    
	printf("サンプル\n");

	for (i = 0; i < argc; ++i) {
		printf("arg[%d]	%s\n", i, argv[i]);
	}
  
    if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			a = argv[i];
			if (*a == '-') {
                option(a);		   			// オプション取得.
			} else {
				doFile(a);
			}
		}
	}

    return 0;
}
