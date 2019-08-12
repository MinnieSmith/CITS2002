#include <stdio.h>
#include <stdlib.h>

int isLeap(int year) {
    if (year % 400 == 0) {
        return 1;
    } else if (year % 100 == 0) {
        return 0;
    } else if (year % 4 == 0) {
        return 1;
    } else {
        return 0;
    }
    }
    



int main(int argcount, char *argvalue[])
{
    int year = atoi(argvalue[1]);

    if (isLeap(year) == 1) {
    printf("%d is a leap year\n", year); 
    }
    else {
     printf("%d is a not leap year\n", year); 
    }
    return 0;

}
