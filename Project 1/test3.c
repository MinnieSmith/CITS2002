#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <math.h>

#define DEBUG_LOG(x, y)                        \
    printf("\n[%s] -------------------\n", x); \
    y printf("[%s] END ---------------\n\n", x)



int calculate_total_completion_time(int x, int tq)
{
    return x*tq;
}

int main (void)
{

    int best_tct[20];

    int n = 0, tq;
    int optimal_time_quantum = 0;
    int min = 20000;
             DEBUG_LOG("BEST TIME QUANTUM LINE 594",
                      printf("MIN: %i\n", min););
    for (tq = 100; tq <= 2000; tq = tq + 100)
    {

        best_tct[n] = calculate_total_completion_time(10, tq);
         DEBUG_LOG("BEST TIME QUANTUM LINE 598",
                      printf("TCT: %i\n", best_tct[n]););
        if (min > best_tct[n])
        {
            min = best_tct[n];
            optimal_time_quantum = tq;
        }
        n++;
    }
    printf("best total process completion time is %i and the optimal time quantum %i\n", min, optimal_time_quantum);
    return 0;


}
