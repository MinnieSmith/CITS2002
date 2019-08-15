#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//  written by Chris.McDonald@uwa.edu.au

//  PROVIDED WITHOUT MUCH EXPLANATION YET!

//  RETURNS  0=Sun, 1=Mon, .....
//
int first_day_of_month(int month, int year)
{
    struct tm   tm;

//  SET ALL FIELDS OF tm TO ZERO TO MAKE SOME FIELDS 'UNKNOWN'
    memset(&tm, 0, sizeof(tm));

//  INITIALIZE THE FILEDS THAT WE ALREADY KNOW
    tm.tm_mday  = 1;
    tm.tm_mon   = month-1;              // 0=Jan, 1=Feb, ....
    tm.tm_year  = year-1900;

//  ASK THE POSIX FUNCTION mktime TO DETERMINE THE 'UNKNOWN' FIELDS
//  See: http://pubs.opengroup.org/onlinepubs/9699919799/
    mktime(&tm);

//  RETURN THE INTEGER MONTH VALUE
    return tm.tm_wday;                  // 0=Sun, 1=Mon, .....
}



#define DEF_MONTH       8
#define DEF_YEAR        2019
#define NWEEKS          6
#define DAYS_PER_WEEK   7

void print_cal(int month, int year) 
{
    int day1 = first_day_of_month(month,year);
    int daysinmonth = 0;

    switch (month) 
    {
    case 1: printf("    January %i\n", year);
        daysinmonth = 31;
        break;
    case 2: printf("    February %i\n", year);
        daysinmonth = 28;
        break;
    case 3: printf("    March %i", year);
        daysinmonth = 31;
        break;
    case 4: printf("    April %i", year);
        daysinmonth = 30;
        break;
    case 5: printf("    May %i", year);
        daysinmonth = 31;
        break;
    case 6: printf("    June %i", year);
        daysinmonth = 30;
        break;
    case 7: printf("    July %i", year);
        daysinmonth = 31;
        break;
    case 8: printf("    August %i", year);
        daysinmonth = 31;
        break;
    case 9: printf("    September %i", year);
        daysinmonth = 30;
        break;
    case 10: printf("   October %i", year);
        daysinmonth = 31;
        break;
    case 11: printf("   November %i", year);
        daysinmonth = 30;
        break;
    case 12: printf("   December %i", year);
        daysinmonth = 31;
        break;
    }

    printf("Su Mo Tu We Th Fr Sa\n");

    int count = 0;
    for(int w=0; w<NWEEKS; ++w){
        for(int d=0; d<DAYS_PER_WEEK; ++d) {
            ++count;
            if(count <= day1 || count - day1 > daysinmonth) {
                printf("  ");
                } else {
                printf("%2i ", count - day1);
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) 
{
    int month = DEF_MONTH;
    int year = DEF_YEAR;

    if(argc > 1) {
        month = atoi(argv[1]);
        if(argc >2) {
            year = atoi(argv[2]);
        }
    }
     print_cal(DEF_MONTH, DEF_YEAR);
     return 0; 
}
