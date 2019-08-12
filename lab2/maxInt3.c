#include <stdio.h>
#include <stdlib.h>
    
int max = 0;

int maxNumber(int value1, int value2, int value3) {

if ((value1 > value2) && (value1 > value3)) {
    max = value1; 
} else if (value2 > value3) {
    max = value2;
} else {
    max = value3;
}
return max;
}

int main (int argcount, char *argvalue[]) {

    int value1 = atoi(argvalue[1]);
    int value2 = atoi(argvalue[2]);
    int value3 = atoi(argvalue[3]);

    maxNumber(value1, value2, value3);

    printf("%d is the maximum value \n", max);

}
