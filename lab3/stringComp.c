#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int my_strcomp(char s[], char t[]) 
{
    int length = 0;
    int sum = 0;
    
    while(s[length] != '0' && t[length] != '0') {
        sum = sum + (s[length] - t[length]);
        length ++;
    }

    if (sum < 0){
        return -1;
    }

    if (sum == 0) {
        return 0;
    }

    if (sum > 0) {
        return 1;
    }
    return 0;
} 

int main(int argc, char *argv[]) 
{
    int result;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s argument\n", argv[0]);
        result = EXIT_FAILURE;
    } else {
        int answer = my_strcomp(argv[1], argv[2]);
        printf("The answer is %d\n", answer);
        result = EXIT_SUCCESS;
    }
    return result;
}
