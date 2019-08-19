#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


int main()
{

    int i = 0;
    int j = 0;
    int n = 10;
    int k = 0;
    int max = 0;   
    int randarr[10];
    int shiftarr[10];
    int length = 10;
    int maxPos = 0;

    srand(time(0));

    for (i = 0; i < n; i++)
    {
        randarr[i] = rand();
        printf("the original array is %i\n", randarr[i]);
    }

    for (k = 0; k < length; k++)
    {
        if (randarr[k] > max)
        {
            max = randarr[k];
            maxPos = k;
        }
    }

    for (j = 0; j < n; j++)
    {
        shiftarr[0] = max;
        if(j<=maxPos) {
        shiftarr[j + 1] = randarr[j];
        } else {
            shiftarr[j] = randarr[j];
        }
        printf("The shifted array is %i\n", shiftarr[j]);
    }

    return 0;

}
