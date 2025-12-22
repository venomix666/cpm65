#include <cpm.h>
#define LEVEL3
#define MEMLEN 10240
void sp() {
    cpm_printstring(" ");
}

void nl() {
    cpm_printstring("\r\n");
}

int printstr (char *s)
{
	int i = 0;
	while (*s){
		//putchar(*s++);
		cpm_conout(*s++);
        i++;
	}
	return i;
}
void cls() {
}

int breakkey() {
    return 0;
}
#define FN fn
