#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int my_strcomp(char s[], char t[])
{
    int length = 0;
    int sum = 0;

    while (s[length] != '\0' || t[length] != '\0')
    {
        sum = sum + (s[length] - t[length]);
        length++;
        printf("%d\n", sum);
    }

    if (sum == 0)
    {
        return 0;
    }

    if (sum > 0)
    {
        return 1;
    }
    if (sum < 0)
    {
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{

        int length = 0;
        int sum = 0;

        while (argv[1][length] != '\0' || argv[2][length] != '\0')
        {
            sum = sum + (argv[1][length] - argv[2][length]);
            length++;
            printf("%d\n", sum);
        }

        if (sum == 0)
        {
            printf("0");
        }

        if (sum > 0)
        {
            printf("1");
        }
        if (sum < 0)
        {
            printf("-1");
        }
    return 0;
}
