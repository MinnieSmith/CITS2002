#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int convert(char c)
{
    return(c - '0');
}

int main (int argcount, char *argvalue[]) {
    int s1 = 0;
    int s2 = 0;
    int i;
    int j;
    int length = strlen(argvalue[1]);

    if(length != 16) {
        printf("Please enter a valid 16 digit credit card number");
    } else {
        for(i = length-1; i >= 0; i = i-2){
            s1 = s1 + convert(argvalue[1][i]);
        }
        for (j = length-2; j>=0; j = j-2) {
            s2 = s2 + (convert(argvalue[1][j])*2);
        }        

        printf("%d\n", s1+s2);
        if(s1+s2 % 10 == 0) {
        printf("This is a valid credit card number");
        } else {
        printf("This is not a valid credit card number");
        }
    }
    
    return 0;
}
