#include <stdio.h>
#include <stdlib.h>

int max = 0;

int main (int argcount, char *argvalue[]) {
    for(int a = 1; a <argcount; a = a + 1) {
        int n = atoi(argvalue[a]);
        if (n > max) {
            max = n;
        }
    }
    printf("%d is the maximum value \n", max);
}
