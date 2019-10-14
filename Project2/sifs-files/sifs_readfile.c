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
    void *data = NULL;
    size_t nbytes = 0;

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

    //  ATTEMPT TO READFILE
    if (SIFS_readfile(volumename, pathname, &data, &nbytes) != 0)
    {
        SIFS_perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }


    // WRITE DATA TO FIILE
    printf("82: Attempt to open file for writing\n");
    FILE *outfp = fopen(filename, "w");
    if (outfp == NULL)
    {
        SIFS_errno = SIFS_ECREATE;
        return 1;
    }
    fwrite(data, nbytes, 1, outfp);
    printf("89:Data written to file\n");

    fclose(outfp);

    // free(&data_buffer);
    return EXIT_SUCCESS;
}
