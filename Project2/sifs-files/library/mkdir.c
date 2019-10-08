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
            return 1;
        }

        // FIND THE NUMBER OF BLOCKS
        fread(&hd, sizeof(SIFS_VOLUME_HEADER), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;
        printf("%i, %zu \n", nblocks, hd.blocksize);

        // SIFS_DIRBLOCK root;
        SIFS_BIT btmp[nblocks];
        SIFS_DIRBLOCK dirblocks;
        int free_block_index = 0;

        // FIND THE INDEX OF THE FIRST FREE BLOCK
        fread(&btmp, sizeof(btmp), 1, fp);
        printf("Bitmap: %s\n", btmp);
        // printf("Number of blocks %i\n", nblocks);


        for (int b = 1; b < (nblocks - 1); ++b)
        {
            // printf("Inloop: %c, %i\n", btmp[b], b);

            if (btmp[b] == SIFS_UNUSED)
            {
                free_block_index = b;
                break;
            }
        }
        printf("free block index %i\n", free_block_index);

        // FIND NUMBER OF DIRECTORIES IN VOLUME
        int ndir = 0;
        for (int i =0; i <nblocks-1; ++i )
        {
            if(btmp[i] == SIFS_DIR)
            {
                ndir++;
            }
        }
        printf("number of directories in volume: %i\n", ndir);


        // FIND THE BLOCK NUMBER OF ALL THE DIRECTORIES IN FILE VIA BITMAP
        int dir_block_number[ndir-1];

        int n = 0;

        {
            for (int b = 1; b < ndir; ++b)
            {
                if (btmp[b] == SIFS_DIR)
                {
                    dir_block_number[n] = b;
                    n++;

                }
            }
        }

        // IF DIRECTORY ALREADY EXISTS
        for (int i = 0; i < ndir-1; ++i)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + blocksize + (blocksize*(i+1));
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            if(strcmp(dirblocks.name, dirname) == 0)
            {
                SIFS_errno = SIFS_EEXIST;
                return 1;
            }
            
        }


        //  UPDATE BITMAP
        btmp[free_block_index] = SIFS_DIR;
        fseek(fp, sizeof(hd), SEEK_SET);
        fwrite(btmp, sizeof(btmp), 1, fp);


        //  CREATE DIRECTORY
        SIFS_DIRBLOCK newdir;
        char oneblock[blocksize];

        // UPDATE VOLUME WITH NEW DIRECTORY
        strcpy(newdir.name, dirname);
        newdir.modtime = time(NULL);
        newdir.nentries = 0;

        int newdir_location = 0;
        newdir_location = sizeof(hd) + sizeof(btmp) + (free_block_index * sizeof(oneblock));

        fseek(fp, newdir_location, SEEK_SET);
        fwrite(&newdir, sizeof(oneblock), 1, fp);

        fclose(fp);
        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
