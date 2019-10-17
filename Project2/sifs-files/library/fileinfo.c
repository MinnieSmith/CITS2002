#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"

//  Written by Minh Smith 20956909 October 2019

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

        // READ BITMAP
        SIFS_BIT btmp[nblocks];
        SIFS_DIRBLOCK dirblocks;
        SIFS_FILEBLOCK fileblock;
        fseek(fp, sizeof(hd), SEEK_SET);
        fread(&btmp, sizeof(btmp), 1, fp);

        // REMOVE ANY '/' FROM PATHNAME
        char *path_tokens[nblocks * SIFS_MAX_NAME_LENGTH];
        char *path = strdup(pathname);
        char *token = strtok(path, "/");
        int t = 0;

        while (token != NULL)
        {
            path_tokens[t] = token;
            token = strtok(NULL, "/");
            t++;
        }

        // FIND NUMBER OF DIRECTORIES IN VOLUME
        int ndir = 0;
        for (int i = 0; i < nblocks - 1; ++i)
        {
            if (btmp[i] == SIFS_DIR)
            {
                ndir++;
            }
        }

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
            for (int i = 1; i < ndir; ++i)
            {
                int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_numbers[i]);
                fseek(fp, dir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                int token = 0;
                do
                {
                    if (strcmp(dirblocks.name, path_tokens[token]) == 0)
                    {
                        subdir_block_id[s] = dir_block_numbers[i];
                        s++;
                    }
                    token++;
                } while (token < (t - 1));
            }
            if (s < (t - 1))
            {
                SIFS_errno = SIFS_EINVAL;
                return 1;
            }
        }

        // CHECK IF PATH IS VALID
        int path_valid_count = 0;
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
                        path_valid_count++;
                    }
                }
            }
        }
        if (path_valid_count < (t - 2))
        {
            SIFS_errno = SIFS_EINVAL;
            return 1;
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
                        // CHECK IF PATH IS VALID PART TWO
                        int block_id_in_search = file_block_number[i];
                        int nreverse = 0;
                        for (int r = 0; r < ndir; ++r)
                        {
                            for (int s = 0; s < ndir; ++s)
                            {
                                int subdir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_numbers[s]);
                                fseek(fp, subdir_location, SEEK_SET);
                                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                                for (int t = 0; t < dirblocks.nentries; t++)
                                {
                                    if (dirblocks.entries[t].blockID == block_id_in_search)
                                    {
                                        block_id_in_search = dir_block_numbers[s];
                                        nreverse++;
                                    }
                                }
                            }
                        }

                        if (nreverse != t)
                        {
                            SIFS_errno = SIFS_EINVAL;
                            return 1;
                        }
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
