#include <stdio.h>
#include <stdlib.h>

int main (int argcount, char *argvalue[]) {
    char ccNumber[17] = argvalue[1];
    int ccLength = strlen(ccNumber);
    printf("%d\n", ccLength);
}
