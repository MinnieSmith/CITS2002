#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "sifs-internal.h"

//  Written by Minh Smith 20956909 October 2019

// read the contents of an existing file from an existing volume
int SIFS_readfile(const char *volumename, const char *pathname,
                  void **data, size_t *nbytes)
{
    //  ENSURE THAT RECEIVED PARAMETERS ARE VALID
    if (volumename == NULL || pathname == NULL || data == NULL || nbytes == NULL)
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
        int dir_block_number[ndir];

        int n = 0;

        for (int b = 0; b < nblocks; ++b)
        {
            if (btmp[b] == SIFS_DIR)
            {
                dir_block_number[n] = b;
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
                int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
                fseek(fp, dir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                int token = 0;
                do
                {
                    if (strcmp(dirblocks.name, path_tokens[token]) == 0)
                    {
                        subdir_block_id[s] = dir_block_number[i];
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

        // CHECK IF PATH IS VALID PART TWO
        if (t > 1)
        {
            // find the directory with first subdirectory
            int block_id_in_search = 0;
            for (int i = 0; i < ndir; ++i)
            {
                int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
                fseek(fp, dir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                if (strcmp(dirblocks.name, path_tokens[t - 2]) == 0)
                {
                    block_id_in_search = dir_block_number[i];
                }
            }

            // trace back the directory path to see if pathname provide is valid
            int nreverse = 0;
            for (int r = 0; r < ndir; ++r)
            {
                for (int s = 0; s < ndir; ++s)
                {
                    int subdir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[s]);
                    fseek(fp, subdir_location, SEEK_SET);
                    fread(&dirblocks, sizeof(dirblocks), 1, fp);
                    for (int t = 0; t < dirblocks.nentries; t++)
                    {
                        if (dirblocks.entries[t].blockID == block_id_in_search)
                        {
                            block_id_in_search = dir_block_number[s];
                            nreverse++;
                        }
                    }
                }
            }
            if (nreverse != (t - 1))
            {
                SIFS_errno = SIFS_EINVAL;
                return 1;
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

        // FIND THE BLOCK NUMBER OF DATA
        int file_block_number[nfile];
        int block_number_of_file = 0;
        int first_data_block = 0;
        int data_size = 0;

        bool file_found = false;
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
            for (int i = 0; i < nfile; ++i)
            {
                int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * file_block_number[i]);
                fseek(fp, file_location, SEEK_SET);
                fread(&fileblock, sizeof(fileblock), 1, fp);
                for (int j = 0; j < SIFS_MAX_ENTRIES; ++j)
                {

                    if (strcmp(fileblock.filenames[j], path_tokens[t - 1]) == 0)
                    {
                        block_number_of_file = file_block_number[i];
                        first_data_block = fileblock.firstblockID;
                        *nbytes = fileblock.length;
                        data_size = fileblock.length;
                        file_found = true;
                        // data = malloc(*nbytes);
                        break;
                    }
                }
            }

            if (!file_found)
            {
                SIFS_errno = SIFS_ENOENT;
                return 1;
            }
        }

        // COPY VOLUME FILE INTO DATA BUFFER
        int data_location = sizeof(hd) + sizeof(btmp) + (blocksize * first_data_block);

        void *data_buffer = NULL;
        data_buffer = malloc(data_size);

        if (data_buffer == NULL)
        {
            SIFS_errno = SIFS_ENOMEM;
            return 1;
        }
        else
        {
            fseek(fp, data_location, SEEK_SET);
            fread(data_buffer, data_size, 1, fp);
        }

        fclose(fp);

        *data = data_buffer;

        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
