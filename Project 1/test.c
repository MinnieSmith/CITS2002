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



//  ----------------------------------------------------------------------
// STRUCTURES DECLARATIONS

struct device
{
    char *name_pointer;
    int bytes_per_sec;
    int priority;
};
struct event
{
    int cpu_time;
    char *device;
    int priority;
    int bytes_transfered;
};

struct process
{
    int id;
    int start_time;
    struct event io_events[MAX_EVENTS_PER_PROCESS];
    int exit_time;
    int time_remaining;
};

struct qnode
{
    struct process *id;
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

int parse_tracefile(char program[], char tracefile[], struct device *d, struct process *p, struct event *e)
{
    int l = 0, m = 0, n = 0;
    int nprocess = 0;

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
            d[l].name_pointer = strdup(word1);
            d[l].bytes_per_sec = atoi(strdup(word2));
            // printf("%i\t%s\t%i\n", l, d[l].name_pointer, d[l].bytes_per_sec);
            l++;
        }

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            ; // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            p[m].id = atoi(strdup(word1));
            p[m].start_time = atoi(strdup(word2));
            // printf("process id%i\tstart time%i\n", p[m].id, p[m].start_time);
            m++;
        }

        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE

            p[nprocess].io_events[n].cpu_time = atoi(strdup(word1));
            p[nprocess].io_events[n].device = strdup(word2);
            p[nprocess].io_events[n].bytes_transfered = atoi(strdup(word3));
            // printf("%i\t%s\t%i\n", p[nprocess].io_events[n].cpu_time,
            //        p[nprocess].io_events[n].device,
            //        p[nprocess].io_events[n].bytes_transfered);
            n++;
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
            p[nprocess].exit_time = atoi(strdup(word1));
            p[nprocess].time_remaining = atoi(strdup(word1));
            // printf("exit\t %i\n", p[nprocess].exit_time);
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
    return nprocess;
}

//  ----------------------------------------------------------------------
// ASSIGN DEVICE PRIORITY

void device_priority_sort(struct device *d)
{
    int k, i, j;
    struct device temp;
    // sort device priority by transfer rate descending

    for (i = 0; i < MAX_DEVICES; i++)
    {
        for (j = 0; j < MAX_DEVICES - 1; j++)
        {
            if (d[j].bytes_per_sec < d[j + 1].bytes_per_sec)
            {
                temp = d[j];
                d[j] = d[j + 1];
                d[j + 1] = temp;
            }
        }
    }

    // assign priority to tracefile_devices struct
    for (k = 0; k < MAX_DEVICES; k++)
    {
        d[k].priority = k + 1;
        printf("%i\t%s\n", d[k].priority, d[k].name_pointer);
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
struct qnode *create_nodes(struct process *id)
{
    struct qnode *temp = (struct qnode *)malloc(sizeof(struct qnode));
    temp->id = id;
    temp->next = NULL;
    return temp;
}

// add node to queue
void enqueue(struct queue *q, struct process *id)
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
void dequeue(struct queue *q)
{
    // store previous front and move head one node down
    q->head = q->head->next;

    if (q->head == NULL)
        q->rear = NULL;
}

// sort nodes
void sort_nodes_in_ready_queue_start_time(struct qnode *head)
{
    struct qnode *i, *j;
    struct process *temp;

    for (i = head; i->next != NULL; i = i->next)
    {
        for (j = i->next; j != NULL; j = j->next)
        {
            if (i->id->start_time > j->id->start_time)
            {
                temp = i->id;
                i->id = j->id;
                j->id = temp;
            }
        }
    }
}

// create READY queue and add processes to nodes
struct queue *create_ready_queue(struct process *p, int nprocess)
{
    int i;
    struct queue *ready = create_queues();
    // loop through and create m (number of processes) nodes
    for (i = 0; i < nprocess; i++)
    {
        enqueue(ready, &p[i]);
    }
    printf("Process %i and %i added \n", ready->head->id->id, ready->rear->id->id);
    //sort nodes: lowest start time at front of ready queue
    sort_nodes_in_ready_queue_start_time(ready->head);

    return ready;
}

int is_queue(struct queue *q)
{
    return q->head != NULL && q->rear != NULL;
}

//  ----------------------------------------------------------------------

// THE CPU

// check to see if context switch is required
int is_context_switch(struct queue *q)
{
    struct process *temp;
    temp = q->head->id;

    sort_nodes_in_ready_queue_start_time(q->head);
    if (q->head->id != temp)
    {
        printf("Process %i and %i sorted \n", q->head->id->id, q->rear->id->id);
    }
    return 1;
}

int is_process_exit(struct queue *q, int tq)
{
    if (q->head->id->time_remaining <= tq)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// calculate total completion time per time quantum
int calculate_total_completion_time(char program[], char tracefile[], int tq)
{
    int total_process_completion_time = 0;
    struct process tracefile_processes[MAX_PROCESSES];
    struct device tracefile_devices[MAX_DEVICES];
    struct event tracefile_events[MAX_EVENTS_PER_PROCESS];

    struct queue *q = create_ready_queue(tracefile_processes, parse_tracefile(program, tracefile, tracefile_devices, tracefile_processes, tracefile_events));
    printf("Process %i and %i is in the queue\n", q->head->id->id, q->head->next->id->id);

    if (is_queue(q))
    {
        total_process_completion_time = q->head->id->start_time + TIME_CONTEXT_SWITCH;
        q->head->id->start_time += TIME_CONTEXT_SWITCH;
    }

    while (is_queue(q))
    {
        if (!is_process_exit(q, tq))
        {
            //execute for 1 tq
            do
            {
                total_process_completion_time += tq;
                q->head->id->time_remaining -= tq;
                q->head->id->start_time += tq;
                printf("1.Process %i is executed and total completion time is %i \n", q->head->id->id, total_process_completion_time);
                // check if there is a context switch or process exit
            } while (!is_context_switch(q) && !is_process_exit(q, tq));

            while (is_context_switch(q) && !is_process_exit(q, tq))
            {
                // execute for another process for 1 tq
                total_process_completion_time += tq + TIME_CONTEXT_SWITCH;
                q->head->id->time_remaining -= tq;
                q->head->id->start_time += tq + TIME_CONTEXT_SWITCH;
                printf("2.Process %i is executed and total completion time is %i \n", q->head->id->id, total_process_completion_time);
            }
        }
        else if (is_process_exit(q, tq))
        {
            total_process_completion_time += q->head->id->time_remaining;
            printf("2.Process %i is completed\n", q->head->id->id);
            dequeue(q);
        }
        else
        {
            printf("unable to calculate function");
            exit(EXIT_FAILURE);
        }
    }
    printf("tct %i\n\n", total_process_completion_time);
    q->head = NULL;
    q->rear = NULL;

    free(q);
    return total_process_completion_time;
}

// find the best total completion time per time quantum
int find_best_time_quantum(char program[], char tracefile[])
{
    int best_tct[20];

    int n = 0, tq;
    int optimal_time_quantum = 0;
    int min = calculate_total_completion_time(program, tracefile, 100);
    for (tq = 100; tq <= 2000; tq = tq + 100)
    {
        best_tct[n] = calculate_total_completion_time(program, tracefile, tq);
        if (best_tct[n] < min)
        {
            min = best_tct[n];
            optimal_time_quantum = tq;
        }
        printf("best tct is %i at %i tq \n", min, optimal_time_quantum);
        n++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // int m = calculate_total_completion_time(argv[0], argv[1], 100);
    // int n = calculate_total_completion_time(argv[0], argv[1], 200);
    // int o = calculate_total_completion_time(argv[0], argv[1], 300);


    // printf("%i\t%i\t%i\n", m,n,o);

    find_best_time_quantum(argv[0], argv[1]);
}
