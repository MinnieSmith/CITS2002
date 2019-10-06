#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "sifs-internal.h"

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *dirname)
{
    //  ENSURE THAT RECEIVED PARAMETERS ARE VALID
    if (volumename == NULL || dirname == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // ENSURE THAT VOLUME EXISTS
    if (access(volumename, F_OK) == 0)
    {

        // SIFS_DIRBLOCK db;
        SIFS_VOLUME_HEADER hd;
        int nblocks;
        int blocksize;


        // ATTEMPT TO OPEN THE VOLUME FOR READ-ONLY ACCESS
        FILE *fp = fopen(volumename, "r+");

        // VOLUME OPEN FAILED
        if (fp == NULL)
        {
            SIFS_errno = SIFS_ENOVOL;
        }



        // FIND THE NUMBER OF BLOCKS
        fread(&hd, sizeof(SIFS_VOLUME_HEADER), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;
        printf("%i, %zu \n", nblocks, hd.blocksize);
        

        // // CHECK TO SEE IF DIRECTORY ALREADY EXISTS
        // fread(&db, sizeof(SIFS_DIRBLOCK), 1, fp);
        // if (strcmp(db.name, dirname) == 0)
        //     SIFS_errno = SIFS_EEXIST;

        // SIFS_DIRBLOCK root;
        SIFS_BIT btmp[nblocks];
        int free_block_index = 0;
       
        // FIND THE INDEX OF THE FIRST FREE BLOCK
        fread(&btmp, sizeof(btmp), 1, fp);
        
        for (int b=1; b<(nblocks-1); ++b)
            {
                if(btmp[b] == 'u')
                    memset(fp, 'd', sizeof(SIFS_BIT));
                    free_block_index = b;
                    break;
            }
            printf("%i\n", free_block_index);        
        

        // FIND THE POSITION OF THE START OF FIRST DIRECTORY
        



        //  CREATE DIRECTORY
        SIFS_DIRBLOCK dirname;
        // char oneblock[blocksize];
        
        //  UPDATE BITMAP USING MEMSET()
        // // btmp[free_block_index] = SIFS_DIR;
        // fseek(fp, 20, SEEK_SET);
        // fwrite(btmp, sizeof(btmp), 1, fp);

        // UPDATE VOLUME WITH NEW DIRECTORY

        dirname.modtime = time(NULL);
        dirname.nentries = 1;

        // memcpy(oneblock, &dirname, sizeof dirname);


        fclose(fp);



    }

    return 0;
}
