#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int my_strlen(char s[]) 
{

    int length = 0;

    while (s[length] != '\0') {
        length ++;
    }
    return length;

}

int main(int argc, char *argv[]) 
{
    int result;

    if (argc <2) {
        fprintf(stderr, "Usage: %s argument\n", argv[0]);
        result = EXIT_FAILURE;
    } else {
        int answer = my_strlen(argv[1]);
        printf("The string length is %d\n", answer);
        result = EXIT_SUCCESS;
    }
    return result;

}
