#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename pathname filename \n", progname);
    fprintf(stderr, "or     %s pathname filename \n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char *volumename; // filename storing the SIFS volume
    char *pathname;
    char *filename;
    size_t nbytes = 0;
    int time_modified = 0;

    //  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if (argcount == 3)
    {
        volumename = getenv("SIFS_VOLUME");
        if (volumename == NULL)
        {
            usage(argvalue[0]);
        }
        pathname = argvalue[1];
        filename = argvalue[2];
    }
    //  ... OR FROM A COMMAND-LINE PARAMETER
    else if (argcount == 4)
    {
        volumename = argvalue[1];
        pathname = argvalue[2];
        filename = argvalue[3];
    }
    else
    {
        usage(argvalue[0]);
        exit(EXIT_FAILURE);
    }

    // GET INFORMATION ABOUT FILE
    struct stat stat_buffer;

    if (stat(filename, &stat_buffer) != 0)
    {
        perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }
    else if (S_ISREG(stat_buffer.st_mode))
    {
        nbytes = stat_buffer.st_size;
        time_modified = (int)stat_buffer.st_mtime;
    }

    printf("datasize: %zu\t modtime: %i\n", nbytes, time_modified);


    // COPY THE FILE INTO DATA BUFFER
    void *data = NULL;
    data = malloc(nbytes);
    if(data == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }
    else
    {
        printf("Attempt to open file for reading\n");
        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }

        fread(data, nbytes, 1, fp);
        if(data == NULL)
        {
            printf("Unable to read file into data");
        }
        fclose(fp);
    }
    

    //  ATTEMPT TO WRITEFILE
        if(SIFS_writefile(volumename, pathname, data, nbytes) != 0) {
    	SIFS_perror(argvalue[0]);
    	exit(EXIT_FAILURE);
        }
        
        free(data);
        return EXIT_SUCCESS;
}
