#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

int isSafe(char s[]) 
{
    int i = 0; 
    int length = strlen(s);
    int u = 0;
    int l = 0;
    int d = 0;

    for(i =0; i < length; i++) {
        if(isupper(s[i]))
        u++;
        if(islower(s[i]))
        l++;
        if(isdigit(s[i]))
        d++;
    }
    
    if(u>=2 && l>=2 & d>=2) {
            printf("Password is safe\n");
        } else {
            printf("Please think of a more secure password\n");
        }
    return 0;
}

int main (int argc, char *argv[]) {
    int result;

    if(argc <2) {
        fprintf(stderr, "Usage: %s argument\n", argv[0]);
        result = EXIT_FAILURE;
    } else {
        isSafe(argv[1]);
        result = EXIT_SUCCESS;
    }
    return result;
}
