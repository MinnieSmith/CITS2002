#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <math.h>

/* CITS2002 Project 1 2019
   Name(s):             Minh Trang Smith
   Student number(s):   20956909
 */

//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

#define MAX_DEVICES 4
#define MAX_DEVICE_NAME 20
#define MAX_PROCESSES 50
// DO NOT USE THIS - #define MAX_PROCESS_EVENTS      1000
#define MAX_EVENTS_PER_PROCESS 100

#define TIME_CONTEXT_SWITCH 5
#define TIME_ACQUIRE_BUS 5

#define DEBUG_LOG(x, y)                        \
    printf("\n[%s] -------------------\n", x); \
    y printf("[%s] END ---------------\n\n", x)

//  ----------------------------------------------------------------------
// STRUCTURES DECLARATIONS
//  ----------------------------------------------------------------------

struct device
{
    char *name_pointer;
    int bytes_per_sec;
    int priority;
} tracefile_devices[MAX_DEVICES];

struct event
{

    int cpu_time;
    int start_time;
    int burst_time;
    char *device;
    int priority;
    int bytes_per_sec;
    int bytes_transfered;
    int event_process_time;
    bool is_finished;
};

struct process
{
    int id;
    int start_time;
    int nevents;
    int nevents_not_finished;
    struct event io_events[MAX_EVENTS_PER_PROCESS];
    int exit_time;
    int time_remaining;
} tracefile_processes[MAX_PROCESSES];

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
// ASSIGN DEVICE PRIORITY
//  ----------------------------------------------------------------------

void device_priority_sort(struct device *d, int ndevices)
{
    int k, i, j;
    struct device temp;
    // sort device priority by transfer rate descending

    for (i = 0; i < ndevices; i++)
    {
        for (j = 0; j < ndevices - 1; j++)
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
    for (k = 0; k < ndevices; k++)
    {
        d[k].priority = k + 1;
    }
}

//  ----------------------------------------------------------------------
//  PARSE TRACEFILE
//  ----------------------------------------------------------------------

#define CHAR_COMMENT '#'
#define MAXWORD 20

int parse_tracefile(char program[], char tracefile[], struct device *d, struct process *p)
{
    int m = 0, n = 0;
    int nprocess = 0;
    int ndevices = 0;

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
            d[ndevices].name_pointer = strdup(word1);
            d[ndevices].bytes_per_sec = atoi(strdup(word2));
            ndevices++;
        }

        else if (nwords == 1 && strcmp(word0, "reboot") == 0)
        {
            device_priority_sort(d, ndevices);
        }

        else if (nwords == 4 && strcmp(word0, "process") == 0)
        {
            // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
            p[m].id = atoi(strdup(word1));
            p[m].start_time = atoi(strdup(word2));
            m++;
        }

        else if (nwords == 4 && strcmp(word0, "i/o") == 0)
        {
            //  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE

            p[nprocess].io_events[n].cpu_time = atoi(strdup(word1));
            p[nprocess].io_events[n].device = strdup(word2);
            p[nprocess].io_events[n].bytes_transfered = atoi(strdup(word3));
            p[nprocess].io_events[n].start_time = atoi(strdup(word1)) + p[nprocess].start_time + TIME_CONTEXT_SWITCH;
            p[nprocess].io_events[n].is_finished = false;

            // calculate burst time
            if (n == 0)
            {
                p[nprocess].io_events[n].burst_time = p[nprocess].io_events[n].cpu_time;
            }
            else
            {
                p[nprocess].io_events[n].burst_time = p[nprocess].io_events[n].cpu_time - p[nprocess].io_events[n - 1].cpu_time;
            }

            // assign device priority and transfer rate per io event
            int i;
            for (i = 0; i < ndevices; i++)
            {
                if (strcmp(p[nprocess].io_events[n].device, d[i].name_pointer) == 0)
                {
                    p[nprocess].io_events[n].priority = d[i].priority;
                    p[nprocess].io_events[n].bytes_per_sec = d[i].bytes_per_sec;
                }
            }
            int x = ceil(p[nprocess].io_events[n].bytes_transfered * 1000000.00 / p[nprocess].io_events[n].bytes_per_sec);
            p[nprocess].io_events[n].event_process_time = x + TIME_ACQUIRE_BUS + TIME_CONTEXT_SWITCH;
            n++;
        }

        else if (nwords == 2 && strcmp(word0, "exit") == 0)
        {
            p[nprocess].exit_time = atoi(strdup(word1));
            p[nprocess].time_remaining = atoi(strdup(word1));
            p[nprocess].nevents = n;
            p[nprocess].nevents_not_finished = n;
            printf("number of io events is %i\n", p[nprocess].nevents);
            printf("exit\t %i\n\n", p[nprocess].exit_time);
            nprocess++;
            n = 0;
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
// CREATE QUEUES AND ADD AND REMOVE NODES
//  ----------------------------------------------------------------------

// create ready queue
struct queue *create_queues()
{
    struct queue *q = (struct queue *)malloc(sizeof(struct queue));
    q->head = q->rear = NULL;
    return q;
}

// create nodes for ready queue
struct qnode *create_nodes(struct process *id)
{
    struct qnode *temp = (struct qnode *)malloc(sizeof(struct qnode));
    temp->id = id;
    temp->next = NULL;
    return temp;
}

// add node to ready queue
void enqueue(struct queue *q, struct process *p)
{

    // create a new node
    struct qnode *temp = create_nodes(p);

    // if queue is empty, head and rear are the same
    if (q->head == NULL)
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

// sort nodes in ready queue
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
struct queue *create_queue(struct process *p, int nprocess)
{
    int i;
    struct queue *ready = create_queues();

    // loop through and create nprocess nodes
    for (i = 0; i < nprocess; i++)
    {

        enqueue(ready, &p[i]);
    }
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
//  ----------------------------------------------------------------------

// check to see if context switch is required
int is_context_switch(struct queue *q)
{
    struct process *temp;
    temp = q->head->id;

    sort_nodes_in_ready_queue_start_time(q->head);
    if (q->head->id != temp)
    {
        return 1;
    }
    else
    {
        return 0;
    }
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

int find_event_index_not_finished(struct queue *q)
{
    int i;
    int index = 0;

    for (i = 0; i < q->head->id->nevents; i++)
    {
        if (q->head->id->io_events[i].is_finished == false)
        {
            index = i;
            break;
        }
    }
    return index;
}
//  ----------------------------------------------------------------------
//  THE BIG CALCULATION
//  ----------------------------------------------------------------------
// calculate total completion time per time quantum
int calculate_total_completion_time(char program[], char tracefile[], int tq)
{
    int total_process_completion_time = 0;
    int first_process_start_time = 0;
    int total_io_completion_time = 0;

    // create the ready queue and io queue
    struct queue *q = create_queue(tracefile_processes, parse_tracefile(program, tracefile, tracefile_devices, tracefile_processes));
    first_process_start_time = q->head->id->start_time;

    //check if there are processes in the READY queue
    if (is_queue(q))
    {
        total_process_completion_time = q->head->id->start_time + TIME_CONTEXT_SWITCH;
        q->head->id->start_time += TIME_CONTEXT_SWITCH;
    }

    while (is_queue(q))
    {
        // if process has io event and burst time is less than tq

        if (q->head->id->nevents_not_finished > 0 && (q->head->id->io_events[find_event_index_not_finished(q)].burst_time <= tq))
        {

            if (q->head->id->start_time > total_process_completion_time)
            {
                total_process_completion_time = q->head->id->start_time;
            }

            DEBUG_LOG("TCT = ST IF TCT < ST LINE 422",
                      printf("ID: %i\n", q->head->id->id);
                      printf("ST: %i\n", q->head->id->start_time);
                      printf("BURST TIME: %i\n", q->head->id->io_events[find_event_index_not_finished(q)].burst_time);
                      printf("Process TCT: %i\n", total_process_completion_time););

            int index = find_event_index_not_finished(q);
            total_process_completion_time += q->head->id->io_events[index].burst_time;
            q->head->id->start_time = total_process_completion_time;

            if (q->head->id->start_time < total_io_completion_time)
            {
                q->head->id->start_time = total_io_completion_time;
            }
            DEBUG_LOG("UPDATE START TIME WITH IO START TIME LINE 436",
                      printf("%i\n", q->head->id->start_time););
            q->head->id->start_time += q->head->id->io_events[index].event_process_time;
            total_io_completion_time = q->head->id->start_time;
            q->head->id->time_remaining -= q->head->id->io_events[index].burst_time;
            q->head->id->io_events[index].is_finished = true;
            q->head->id->nevents_not_finished -= 1;

            DEBUG_LOG("IF IO EVENT AND BT <= TQ LINE 444",
                      printf("Process ID: %i\n", q->head->id->id);
                      printf("IO PROCESSING TIME %i\n", q->head->id->io_events[index].event_process_time);
                      printf("TCT: %i\n", total_process_completion_time);
                      printf("TIOCT: %i\n", total_io_completion_time);
                      printf("TR: %i\n", q->head->id->time_remaining);
                      printf("START TIME AFTER IO: %i\n", q->head->id->start_time););
        }
        
        // if process has io event and burst time is greater than 1 tq
        else if (q->head->id->nevents_not_finished > 0 && (q->head->id->io_events[find_event_index_not_finished(q)].burst_time > tq))
        {
            int index = find_event_index_not_finished(q);
            total_process_completion_time += tq;
            q->head->id->start_time = total_process_completion_time;
            q->head->id->io_events[index].burst_time -= tq;
            q->head->id->time_remaining -= tq;
            DEBUG_LOG("IF IO EVENT AND BT > TQ LINE 471",
                      printf("Process ID: %i\n", q->head->id->id);
                      printf("INDEX %i\n", index);
                      printf("TCT: %i\n", total_process_completion_time);
                      printf("TR: %i\n", q->head->id->time_remaining);
                      printf("START TIME: %i\n", q->head->id->start_time););
            if (is_context_switch(q))
            {
                total_process_completion_time += TIME_CONTEXT_SWITCH;
                DEBUG_LOG("CONTEXT SWITCH 480",
                          printf("TCT: %i\n", total_process_completion_time););
            }
        }
        // if process has no io event and time remaining is greater than 1 tq
        else if (q->head->id->nevents_not_finished == 0 && (q->head->id->time_remaining > tq))
        {
            if (q->head->id->start_time > total_process_completion_time)
            {
                total_process_completion_time = q->head->id->start_time;
            }
            total_process_completion_time += tq;
            q->head->id->start_time = total_process_completion_time;
            q->head->id->time_remaining -= tq;
            DEBUG_LOG("IF NO EVENT AND TR > TQ LINE 494",
                      printf("Process ID: %i\n", q->head->id->id);
                      printf("TCT: %i\n", total_process_completion_time);
                      printf("TR: %i\n", q->head->id->time_remaining);
                      printf("START TIME: %i\n", q->head->id->start_time););

            if (is_context_switch(q) || (q->head == q->rear))
            {
                total_process_completion_time += TIME_CONTEXT_SWITCH;
                DEBUG_LOG("CONTEXT SWITCH 503",
                          printf("TCT: %i\n", total_process_completion_time););
            }
        }
        // if process has no io event and time remaining is less than 1 tq
        else if (q->head->id->nevents_not_finished == 0 && (q->head->id->time_remaining <= tq))
        {
            if (q->head->id->start_time > total_process_completion_time)
            {
                total_process_completion_time = q->head->id->start_time;
            }

            total_process_completion_time += q->head->id->time_remaining;

            DEBUG_LOG("PROCESS EXIT LINE 517",
                      printf("TCT: %i\n", total_process_completion_time);
                      printf("PROCESS %i COMPLETED\n", q->head->id->id););

            dequeue(q);
            if (q->head != NULL)
            {
                if (total_process_completion_time < total_io_completion_time)
                {
                    total_process_completion_time = total_io_completion_time;
                }
                total_process_completion_time += TIME_CONTEXT_SWITCH;
            }

            DEBUG_LOG("PROCESS EXIT Line 531",
                      printf("TCT: %i\n", total_process_completion_time););
        }

        else
        {
            printf("unable to calculate function\n");
            exit(EXIT_FAILURE);
        }
    }

    total_process_completion_time -= first_process_start_time;
    q->head = NULL;
    q->rear = NULL;

    free(q);
    DEBUG_LOG("BEST TCT LINE 547",
              printf("TCT: %i\n", total_process_completion_time););
    return total_process_completion_time;
}

// find the best total completion time per time quantum
int find_best_time_quantum(char program[], char tracefile[], char *tq, char *maxtq, char *increment)
{

    int n = 0, i = 0;
    int tq_int = atoi(tq);
    int maxtq_int = atoi(maxtq);
    int increment_int = atoi(increment);
    int size = ((maxtq_int - tq_int) / increment_int) + 1;
    int best_tct[size];
    int optimal_time_quantum = tq_int;

    int min = calculate_total_completion_time(program, tracefile, tq_int);

    for (i = tq_int; i <= maxtq_int; i = i + increment_int)
    {
        best_tct[n] = calculate_total_completion_time(program, tracefile, i);

        if (best_tct[n] <= min)
        {
            min = best_tct[n];
            optimal_time_quantum = i;
        }
        n++;
    }
    printf("best %i %i\n", optimal_time_quantum, min);
    return 0;
}

//  ------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "%s: program expected 1 argument, but instead received %d\n",
                argv[0], argc - 1);
        exit(EXIT_FAILURE);
    }
    else
    {
        find_best_time_quantum(argv[0], argv[1], argv[2], argv[3], argv[4]);
        exit(EXIT_SUCCESS);
    }
    return 0;
}
