#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* CITS2002 Project 1 2019
   Name(s):             Minh Trang Smith
   Student number(s):   20956909
 */

//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

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

int total_process_completion_time = 0;
int optimal_time_quantum = 0;

int nprocess = 0;
int l = 0, m = 0, n = 0;

//  ----------------------------------------------------------------------
// STRUCTURES DECLARATIONS

struct device
{
    char *name_pointer;
    int bytes_per_sec;
    int priority;
} tracefile_devices[MAX_DEVICES], temp;

struct process
{
    int id;
    int start_time;
    struct event
    {
        int cpu_time;
        char *device;
        int priority;
        int bytes_transfered;
    } tracefile_events[MAX_EVENTS_PER_PROCESS];
    int exit_time;
    int time_remaining;
} tracefile_processes[MAX_PROCESSES], temp_process;

struct qnode
{
    struct process id;
    struct qnode *next;
};

struct queue
{
    struct qnode *head;
    struct qnode *rear;
};

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

        // struct event tracefile_events;
        if (nwords == 4 && strcmp(word0, "device") == 0)
        // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
        {
            tracefile_devices[l].name_pointer = strdup(word1);
            tracefile_devices[l].bytes_per_sec = atoi(strdup(word2));
            printf("%i\t%s\t%i\n", l, tracefile_devices[l].name_pointer, tracefile_devices[l].bytes_per_sec);
            l++;
        }

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            ; // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            tracefile_processes[m].id = atoi(strdup(word1));
            tracefile_processes[m].start_time = atoi(strdup(word2));
            printf("process id%i\tstart time%i\n", tracefile_processes[m].id, tracefile_processes[m].start_time);
            m++;
        }

        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE

            tracefile_processes[nprocess].tracefile_events[n].cpu_time = atoi(strdup(word1));
            tracefile_processes[nprocess].tracefile_events[n].device = strdup(word2);
            tracefile_processes[nprocess].tracefile_events[n].bytes_transfered = atoi(strdup(word3));
            printf("%i\t%s\t%i\n", tracefile_processes[nprocess].tracefile_events[n].cpu_time,
                   tracefile_processes[nprocess].tracefile_events[n].device,
                   tracefile_processes[nprocess].tracefile_events[n].bytes_transfered);
            n++;
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
            tracefile_processes[nprocess].exit_time = atoi(strdup(word1));
            printf("exit\t %i\n", tracefile_processes[nprocess].exit_time);
            nprocess++;
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

//  ----------------------------------------------------------------------
// ASSIGN DEVICE PRIORITY

void device_priority_sort()
{
    int k, i, j;
    // sort device priority by transfer rate descending

    for (j = 0; j < MAX_DEVICES - 1; j++)
    {
        if (tracefile_devices[j].bytes_per_sec < tracefile_devices[j + 1].bytes_per_sec)
        {
            temp = tracefile_devices[j];
            tracefile_devices[j] = tracefile_devices[j + 1];
            tracefile_devices[j + 1] = temp;
        }
    }

    // assign priority to tracefile_devices struct
    for (k = 0; k < MAX_DEVICES; k++)
    {
        tracefile_devices[k].priority = k + 1;
        printf("%i\t%s\n", tracefile_devices[k].priority, tracefile_devices[k].name_pointer);
    }
}

//  ----------------------------------------------------------------------
// CREATE QUEUES AND ADD AND REMOVE NODES

// create queues
struct queue *create_queues()
{
    struct queue *q = (struct queue *)malloc(sizeof(struct queue));
    q->head = q->rear = NULL;
    return q;
}

// create nodes
struct qnode *create_nodes(struct process id)
{
    struct qnode *temp = (struct qnode *)malloc(sizeof(struct qnode));
    temp->id = id;
    temp->next = NULL;
    return temp;
}

// add node to queue
void enqueue(struct queue *q, struct process id)
{
    // create a new node
    struct qnode *temp = create_nodes(id);
    // if queue is empty, head and rear are the same
    if (q->rear == NULL)
    {
        q->head = q->rear = temp;
        return;
    }
    // add the new node at the rear and change rear
    q->rear->next = temp;
    q->rear = temp;
}

// remove node from head
struct qnode *dequeue(struct queue *q)
{
    // checks to see if the queue is empty
    if (q->head == NULL)
    {
        return NULL;
    }
    // store previous front and move head one node down
    struct qnode *temp = q->head;
    q->head = q->head->next;

    if (q->head == NULL)
        q->rear = NULL;
    return temp;
}

// sort nodes
void sort_nodes_for_ready_queue()
{
    int j;
    for (j = 0; j < m; j++)
    {
        if (qnode[j]->id > tracefile_processes[j + 1].start_time)
        {
            temp_process = tracefile_processes[j];
            tracefile_processes[j] = tracefile_processes[j + 1];
            tracefile_processes[j + 1] = temp_process;
        }
    }
}

// create READY queue and add processes to nodes
struct queue *create_ready_queue()
{
    int i, j, k;
    struct queue *ready = create_queues();
    // loop through and create m (number of processes) nodes
    for (i = 0; i < m; i++)
    {
        create_nodes(tracefile_processes[i]);
    }

    sort_nodes_for_ready_queue();

    return ready;
}

//  ----------------------------------------------------------------------
// CREATE THE CPU

//

int main(int argc, char *argv[])
{
    parse_tracefile(argv[0], argv[1]);
    device_priority_sort();
    create_ready_queue();
}
