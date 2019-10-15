#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
                  size_t *length, time_t *modtime)
{
    //  ENSURE THAT RECEIVED PARAMETERS ARE VALID
    if (volumename == NULL || pathname == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // ENSURE THAT VOLUME EXISTS
    if (access(volumename, F_OK) == 0)
    {

        // ATTEMPT TO OPEN THE VOLUME
        FILE *fp = fopen(volumename, "r");
        printf("28:volume address %p\n", (void *)fp);

        // VOLUME OPEN FAILED
        if (fp == NULL)
        {
            SIFS_errno = SIFS_ENOVOL;
            return 1;
        }

        // FIND THE NUMBER OF BLOCKS IN VOLUME
        SIFS_VOLUME_HEADER hd;
        int nblocks;
        int blocksize;
        fread(&hd, sizeof(hd), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;
        printf("%i, %zu \n", nblocks, hd.blocksize);

        // READ BITMAP
        SIFS_BIT btmp[nblocks];
        SIFS_DIRBLOCK dirblocks;
        SIFS_FILEBLOCK fileblock;
        fseek(fp, sizeof(hd), SEEK_SET);
        fread(&btmp, sizeof(btmp), 1, fp);
        printf("Bitmap: %s\n", btmp);

        // REMOVE ANY '/' FROM PATHNAME
        char *path_tokens[nblocks * SIFS_MAX_NAME_LENGTH];
        char *path = strdup(pathname);
        char *token = strtok(path, "/");
        int t = 0;

        while (token != NULL)
        {
            path_tokens[t] = token;
            printf("token: %s\n", path_tokens[t]);
            token = strtok(NULL, "/");
            t++;
        }
        printf("t = %i \n", t);

        // FIND NUMBER OF DIRECTORIES IN VOLUME
        int ndir = 0;
        for (int i = 0; i < nblocks - 1; ++i)
        {
            if (btmp[i] == SIFS_DIR)
            {
                ndir++;
            }
        }
        printf("number of directories in volume: %i\n", ndir);

        // FIND THE BLOCK NUMBER OF ALL THE DIRECTORIES IN FILE VIA BITMAP
        int dir_block_numbers[ndir];

        int n = 0;

        for (int b = 0; b < nblocks; ++b)
        {
            if (btmp[b] == SIFS_DIR)
            {
                dir_block_numbers[n] = b;
                n++;
            }
        }

        // CHECK IF SUBDIRECTORY EXISTS
        int subdir_block_id[t - 1];
        int s = 0;

        if (t > 1)
        {
            printf("t = %i \n", t);
            for (int i = 1; i < ndir; ++i)
            {
                int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_numbers[i]);
                printf("dirblocknumber: %i\n", dir_block_numbers[i]);
                printf("dir_location: %i\n", dir_location);
                fseek(fp, dir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                int token = 0;
                do
                {
                    if (strcmp(dirblocks.name, path_tokens[token]) == 0)
                    {
                        subdir_block_id[s] = dir_block_numbers[i];
                        printf("s= %i\n", subdir_block_id[s]);
                        s++;
                    }
                    token++;
                } while (token < (t - 1));
            }
            if (s < (t - 1))
            {
                SIFS_errno = SIFS_EINVAL;
                printf("No such subdirectory, please check pathname\n");
                return 1;
            }
        }

        // CHECK IF PATH IS VALID
        if (t > 2)
        {
            for (int i = 0; i < (s - 1); ++i)
            {
                int subdir_location = sizeof(hd) + sizeof(btmp) + (blocksize * subdir_block_id[i]);
                fseek(fp, subdir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                for (int j = 0; j < dirblocks.nentries; j++)
                {
                    if (dirblocks.entries[j].blockID == subdir_block_id[i + 1])
                    {
                        break;
                    }
                    else
                    {
                        SIFS_errno = SIFS_EINVAL;
                        printf("Path incorrect, please check pathname\n");
                        return 1;
                    }
                }
            }
        }

        // FIND NUMBER OF FILES IN VOLUME
        int nfile = 0;
        for (int i = 0; i < nblocks; ++i)
        {
            if (btmp[i] == SIFS_FILE)
            {
                nfile++;
            }
        }
        printf("number of files in volume: %i\n", nfile);

        // FIND THE BLOCK NUMBER OF THE FILES
        int file_block_number[nfile];

        int f = 0;
        if (nfile > 0)
        {
            for (int b = 1; b < nblocks; ++b)
            {
                if (btmp[b] == SIFS_FILE)
                {
                    file_block_number[f] = b;
                    printf("fileblock number: %i\n", file_block_number[f]);
                    f++;
                }
            }

            // CHECK IF LAST TOKEN OF PATHNAME IS A FILE
            bool file_found = false;
            for (int i = 0; i < nfile; ++i)
            {
                int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * file_block_number[i]);
                fseek(fp, file_location, SEEK_SET);
                fread(&fileblock, sizeof(fileblock), 1, fp);
                for (int j = 0; j < SIFS_MAX_ENTRIES; ++j)
                {

                    if (strcmp(fileblock.filenames[j], path_tokens[t - 1]) == 0)
                    {
                        *length = fileblock.length;
                        *modtime = fileblock.modtime;
                        file_found = true;
                    }
                }
            }

            if (!file_found)
            {
                SIFS_errno = SIFS_ENOTFILE;
                return 1;
            }
        }

        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
