#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* CITS2002 Project 1 2019
   Name(s):             student-name1 (, student-name2)
   Student number(s):   student-number-1 (, student-number-2)
 */

//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c

//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES 4
#define MAX_DEVICE_NAME 20
#define MAX_PROCESSES 50
// DO NOT USE THIS - #define MAX_PROCESS_EVENTS      1000
#define MAX_EVENTS_PER_PROCESS 100

#define TIME_CONTEXT_SWITCH 5
#define TIME_ACQUIRE_BUS 5

//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum = 0;
int total_process_completion_time = 0;

struct device
{
    char *name_pointer;
    int bytes_per_sec;
<<<<<<< HEAD
} tracefile_devices[MAX_DEVICES], temp;
||||||| merged common ancestors
};

struct event
{
    int cpu_time;
    char *device;
    int bytes_transfered;
};
=======
} tracefile_devices;

struct event
{
    int cpu_time;
    char *device;
    int bytes_transfered;
} tracefile_events;
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20

struct process
{
<<<<<<< HEAD
    int id;
||||||| merged common ancestors
=======
    int process_id;
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
    int start_time;
<<<<<<< HEAD
    struct event
    {
        int cpu_time;
        char *device;
        int priority;
        int bytes_transfered;
    } tracefile_events[MAX_EVENTS_PER_PROCESS];
||||||| merged common ancestors
    // struct sevent;
=======
    // struct tracefile_events;
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
    int exit_time;
<<<<<<< HEAD
} tracefile_processes[MAX_PROCESSES];
||||||| merged common ancestors
};
=======
} tracefile_processes;
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20

//  ----------------------------------------------------------------------

#define CHAR_COMMENT '#'
#define MAXWORD 20

void parse_tracefile(char program[], char tracefile[])
{
    //  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp = fopen(tracefile, "r");

    if (fp == NULL)
    {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int lc = 0;
    int nprocess = 0;

    //  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while (fgets(line, sizeof line, fp) != NULL)
    {
        ++lc;

        //  COMMENT LINES ARE SIMPLY SKIPPED
        if (line[0] == CHAR_COMMENT)
        {
            continue;
        }

        //  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);
        // printf("%i = %s", nwords, line);

        //  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if (nwords <= 0)
        {
            continue;
        }
        //  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENT
<<<<<<< HEAD

||||||| merged common ancestors
        struct device tracefile_devices;
=======
        
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
        // struct event tracefile_events;
        if (nwords == 4 && strcmp(word0, "device") == 0)
        // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
        {
<<<<<<< HEAD
            int n = 0;
            tracefile_devices[n].name_pointer = strdup(word1);
            tracefile_devices[n].bytes_per_sec = atoi(strdup(word2));
            printf("%s\t%i\n", tracefile_devices[n].name_pointer, tracefile_devices[n].bytes_per_sec);
            n++;
||||||| merged common ancestors
            tracefile_devices.name_pointer = word1;
            tracefile_devices.bytes_per_sec = atoi(word2);
            printf("%s\n", tracefile_devices.name_pointer);
            printf("%i\n", tracefile_devices.bytes_per_sec);
=======
            tracefile_devices.name_pointer = strdup(word1);
            tracefile_devices.bytes_per_sec = atoi(strdup(word2));
            printf("%s\t%i\n", tracefile_devices.name_pointer, tracefile_devices.bytes_per_sec);
        }

        else if(nwords == 1 && strcmp(word0, "reboot") == 0) {
            ;   // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }
        else if(nwords == 4 && strcmp(word0, "process") == 0) {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            tracefile_processes.process_id = atoi(strdup(word1));
            tracefile_processes.start_time = atoi(strdup(word2));
            printf("%i\t%i\n", tracefile_processes.process_id, tracefile_processes.start_time);
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
        }
<<<<<<< HEAD

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            ; // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            int n = 0;
            tracefile_processes[n].id = atoi(strdup(word1));
            tracefile_processes[n].start_time = atoi(strdup(word2));
            printf("process id%i\tstart time%i\n", tracefile_processes[n].id, tracefile_processes[n].start_time);
            n++;
        }

||||||| merged common ancestors
        
=======
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
<<<<<<< HEAD
            int n = 0;
            tracefile_processes[nprocess].tracefile_events[n].cpu_time = atoi(strdup(word1));
            tracefile_processes[nprocess].tracefile_events[n].device = strdup(word2);
            tracefile_processes[nprocess].tracefile_events[n].bytes_transfered = atoi(strdup(word3));
            printf("%i\t%s\t%i\n", tracefile_processes[nprocess].tracefile_events[n].cpu_time,
                   tracefile_processes[nprocess].tracefile_events[n].device,
                   tracefile_processes[nprocess].tracefile_events[n].bytes_transfered);
||||||| merged common ancestors
            // tracefile_events.cpu_time = atoi(word1);
            // tracefile_events.device = word2;
            // tracefile_events.bytes_transfered = atoi(word3);
            // printf("%i\n%s\n%i\n", tracefile_events.cpu_time, tracefile_events.device, tracefile_events.bytes_transfered);
            printf("%s\n", word1);
=======
            tracefile_events.cpu_time = atoi(strdup(word1));
            tracefile_events.device = strdup(word2);
            tracefile_events.bytes_transfered = atoi(strdup(word3));
            printf("%i\t%s\t%i\n", tracefile_events.cpu_time, tracefile_events.device, tracefile_events.bytes_transfered);

>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
<<<<<<< HEAD
            tracefile_processes[nprocess].exit_time = atoi(strdup(word1));
            printf("exit\t %i\n", tracefile_processes[nprocess].exit_time);
            nprocess++;
||||||| merged common ancestors
            ; //  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
=======
            tracefile_processes.exit_time = atoi(strdup(word1));
            printf("Exit time is %i\n", tracefile_processes.exit_time);
>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
        }

        else if (nwords == 1 && strcmp(word0, "}") == 0)
        {
            ; //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
        }
        else
        {
            printf("%s: line %i of '%s' is unrecognized",
                   program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
}

void device_priority_sort()
{
<<<<<<< HEAD

    for (int i = 0; i < MAX_DEVICES - 1; i++)
    {
        if (tracefile_devices[i].bytes_per_sec > tracefile_devices[i + 1].bytes_per_sec)
        {
            temp = tracefile_devices[i];
            tracefile_devices[i] = tracefile_devices[i + 1];
            tracefile_devices[i + 1] = temp;
        }
    }

    for (int i = 0; i < MAX_DEVICES; i++)
    {
        printf("%s\t%i\n", tracefile_devices[i].name_pointer, tracefile_devices[i].bytes_per_sec);
    }
||||||| merged common ancestors
    parse_tracefile(argv[0], argv[1]);
=======
    parse_tracefile(argv[0], argv[1]);

>>>>>>> ad1fa85395d16c6d0103757d2e7d30fa2c70dd20
}

    int main(int argc, char *argv[])
    {
        parse_tracefile(argv[0], argv[1]);
        device_priority_sort();
    }
