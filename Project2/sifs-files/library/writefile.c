#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "sifs-internal.h"

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
                   void *data, size_t nbytes)
{
    //  ENSURE THAT RECEIVED PARAMETERS ARE VALID
    if (volumename == NULL || pathname == NULL || data == NULL || nbytes < 0)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // ENSURE THAT VOLUME EXISTS
    if (access(volumename, F_OK) == 0)
    {

        // ATTEMPT TO OPEN THE VOLUME
        FILE *fp = fopen(volumename, "r+");

        // VOLUME OPEN FAILED
        if (fp == NULL)
        {
            SIFS_errno = SIFS_ENOVOL;
            return 1;
        }

        // FIND THE NUMBER OF BLOCKS
        SIFS_VOLUME_HEADER hd;
        int nblocks;
        int blocksize;
        fread(&hd, sizeof(hd), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;
        printf("%i, %zu \n", nblocks, hd.blocksize);

        // FIND THE NUMBER OF FREE BLOCKS
        SIFS_BIT btmp[nblocks];
        SIFS_DIRBLOCK dirblocks;
        SIFS_FILEBLOCK fileblock;

        fseek(fp, sizeof(hd), SEEK_SET);
        fread(&btmp, sizeof(btmp), 1, fp);
        printf("Bitmap: %s\n", btmp);

        int free_block_count = 0;
        for (int b = 1; b < nblocks; ++b)
        {
            // printf("Inloop: %c, %i\n", btmp[b], b);

            if (btmp[b] == SIFS_UNUSED)
            {
                free_block_count++;
            }
        }

        // CALCULATE THE NUMBER OF BLOCKS REQURIED FOR DATA
        int data_blocks_required = ceil(nbytes / blocksize);
        int total_blocks_required = data_blocks_required +1;

        if(total_blocks_required>free_block_count)
        {
            SIFS_errno = SIFS_ENOSPC;
            return 1;
        }

        // FIND INDEX OF FIRST FIRST BLOCK WITH ADEQUATE CONTIGUOUS BLOCKS
        int unused_block_count = 0;
        int free_contiguous_block_number = 0; // block number of where the contiguous block starts
        bool enough_datablocks = false;
        for (int b = 1; b < nblocks; ++b)
        {
            if (btmp[b] == SIFS_UNUSED)
            {
                unused_block_count++;
                if (unused_block_count == data_blocks_required)
                {
                    free_contiguous_block_number = b;
                    enough_datablocks = true;
                }
            }
            else
            {
                unused_block_count = 0;
            }
        }

        if (!enough_datablocks)
        {
            SIFS_errno = SIFS_ENOSPC;
        }

        // TODO: WRITE DATA TO DATABLOCKS

        // FIND INDEX OF FREE BLOCK FOR FILEBLOCK ASSIGNMENT
        int free_block_index = 0;
        for (int b = 1; b < nblocks; ++b)
        {
            // printf("Inloop: %c, %i\n", btmp[b], b);

            if (btmp[b] == SIFS_UNUSED)
            {
                free_block_index = b;
                break;
            }
        }

        // CHECK TO SEE IF THERE ARE ANY FREE BLOCKS
        if (free_block_index == 0)
        {
            SIFS_errno = SIFS_ENOSPC;
            return 1;
        }
        printf("free block index %i\n", free_block_index);

        // TODO: ASSIGN FILE BLOCK

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

        // CHECK IF LAST TOKEN IS A FILE
        // FIND NUMBER OF FILES IN VOLUME
        int nfile = 0;
        for (int i = 0; i < nblocks - 1; ++i)
        {
            if (btmp[i] == SIFS_FILE)
            {
                nfile++;
            }
        }
        printf("number of files in volume: %i\n", nfile);

        // FIND THE BLOCK NUMBER OF ALL THE FILE VIA BITMAP
        int file_block_number[nfile];
        int f = 0;
        if (nfile > 0)
        {
            for (int b = 1; b < nfile; ++b)
            {
                if (btmp[b] == SIFS_FILE)
                {
                    file_block_number[f] = b;
                    printf("fileblock number: %i\n", file_block_number[f]);
                    f++;
                }
            }

            // CHECK IF ANY TOKEN OF PATHNAME IS A FILE
            for (int i = 0; i < nfile; ++i)
            {
                int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * file_block_number[i]);
                fseek(fp, file_location, SEEK_SET);
                fread(&fileblock, sizeof(fileblock), 1, fp);
                for (int j = 0; j < SIFS_MAX_ENTRIES; ++j)
                {
                    for (int k = 0; k < t; ++k)
                    {
                        if (strcmp(fileblock.filenames[j], path_tokens[k]) == 0)
                        {
                            SIFS_errno = SIFS_ENOTDIR;
                            return 1;
                        }
                    }
                }
            }
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
        printf("number of directories in volume: %i\n", ndir);

        // FIND THE BLOCK NUMBER OF ALL THE DIRECTORIES IN FILE VIA BITMAP
        int dir_block_number[ndir];

        int n = 0;

        for (int b = 0; b < ndir; ++b)
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
            printf("t = %i \n", t);
            printf("size of header: %lu \t size of bitmap: %lu\t size of rootblock: %i\n", sizeof(hd), sizeof(btmp), blocksize);
            for (int i = 0; i < ndir; ++i)
            {
                int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
                printf("dirblocknumber: %i\n", dir_block_number[i]);
                printf("dir_location: %i\n", dir_location);
                fseek(fp, dir_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                int token = 0;
                do
                {
                    if (strcmp(dirblocks.name, path_tokens[token]) == 0)
                    {
                        subdir_block_id[s] = dir_block_number[i];
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

        // CHECK IF DIRECTORY NAME ALREADY EXISTS
        for (int i = 0; i < ndir; i++)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            printf("dirblock name: %s\n", dirblocks.name);
            if (strcmp(dirblocks.name, path_tokens[t - 1]) == 0)
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
        strcpy(newdir.name, path_tokens[t - 1]);
        newdir.modtime = time(NULL);
        newdir.nentries = 0;

        int newdir_location = 0;
        newdir_location = sizeof(hd) + sizeof(btmp) + (free_block_index * blocksize);

        fseek(fp, newdir_location, SEEK_SET);
        fwrite(&newdir, sizeof(oneblock), 1, fp);

        // UPDATE THE ROOT
        if (t == 1)
        {
            int root_location = sizeof(hd) + sizeof(btmp);
            fseek(fp, root_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            dirblocks.entries[dirblocks.nentries].blockID = free_block_index;
            dirblocks.nentries += 1;
            fseek(fp, root_location, SEEK_SET);
            fwrite(&dirblocks, sizeof(dirblocks), 1, fp);
        }

        // UPDATE ENTRY ON SUBDIRECTORY
        if (t > 1)
        {
            SIFS_DIRBLOCK subdir_to_be_updated;
            int subdir_to_be_updated_location = sizeof(hd) + sizeof(btmp) + (blocksize * subdir_block_id[s - 1]);
            fseek(fp, subdir_to_be_updated_location, SEEK_SET);
            fread(&subdir_to_be_updated, sizeof(subdir_to_be_updated), 1, fp);

            if (subdir_to_be_updated.nentries == SIFS_MAX_ENTRIES)
            {
                SIFS_errno = SIFS_EMAXENTRY;
                return 1;
            }

            subdir_to_be_updated.entries[subdir_to_be_updated.nentries].blockID = free_block_index;
            subdir_to_be_updated.nentries += 1; // update number of entries
            fseek(fp, subdir_to_be_updated_location, SEEK_SET);
            fwrite(&subdir_to_be_updated, sizeof(subdir_to_be_updated), 1, fp);
        }
        fclose(fp);
        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
