#include <stdio.h>
#include "subr.h"

int main(int argc, char* argv[]) {
    // ������̂�����ɋ�.        
    
	printf("�T���v��\n");

	for (i = 0; i < argc; ++i) {
		printf("arg[%d]	%s\n", i, argv[i]);
	}
  
    if (argc > 1) {
		for (int i = 1; i < argc; ++i) {
			a = argv[i];
			if (*a == '-') {
                option(a);		   			// �I�v�V�����擾.
			} else {
				doFile(a);
			}
		}
	}

    return 0;
}
