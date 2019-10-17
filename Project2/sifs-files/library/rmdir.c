#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"

//  Written by Minh Smith 20956909 October 2019

// make a new directory within an existing volume
int SIFS_rmdir(const char *volumename, const char *dirname)
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
        fread(&hd, sizeof(SIFS_VOLUME_HEADER), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;

        // FIND THE INDEX OF THE FIRST FREE BLOCK
        SIFS_BIT btmp[nblocks];
        SIFS_DIRBLOCK dirblocks;
        SIFS_FILEBLOCK fileblock;
        int free_block_index = 0;

        fread(&btmp, sizeof(btmp), 1, fp);

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

        // REMOVE ANY '/' FROM PATHNAME
        char *path_tokens[nblocks * SIFS_MAX_NAME_LENGTH];
        char *path = strdup(dirname);
        char *token = strtok(path, "/");
        int t = 0;

        while (token != NULL)
        {
            path_tokens[t] = token;
            token = strtok(NULL, "/");
            t++;
        }

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

        // FIND THE BLOCK NUMBER OF ALL THE FILE VIA BITMAP
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
            for (int i = 0; i < ndir; ++i)
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

        // CHECK IF DIRECTORY NAME EXISTS
        bool dirname_exists = false;
        char oneblock[blocksize];
        for (int i = 0; i < ndir; i++)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            if (strcmp(dirblocks.name, path_tokens[t - 1]) == 0)
            {
                // CHECK IF PATH IS VALID PART TWO
                int block_id_in_search = dir_block_number[i];
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
                if (nreverse != t)
                    {
                        SIFS_errno = SIFS_EINVAL;
                        return 1;
                    }

                //CHECK IF DIRECTORY IS EMPTY REMOVE DIRECTORY AND UPDATE BITMAP
                dirname_exists = true;
                if (dirblocks.nentries > 0)
                {
                    SIFS_errno = SIFS_ENOTEMPTY;
                    return 1;
                }

                if (dirblocks.nentries == 0)
                {
                    fseek(fp, dir_location, SEEK_SET);
                    memset(oneblock, 0, sizeof(oneblock));   // reset block to zero
                    btmp[dir_block_number[i]] = SIFS_UNUSED; // update bitmap
                    fseek(fp, sizeof(hd), SEEK_SET);
                    fwrite(btmp, sizeof(btmp), 1, fp);

                    // UPDATE THE ROOT
                    if (t == 1)
                    {
                        int root_location = sizeof(hd) + sizeof(btmp);
                        fseek(fp, root_location, SEEK_SET);
                        fread(&dirblocks, sizeof(dirblocks), 1, fp);
                        // FIND THE ENTRY NUMBER
                        for (int j = 0; j < nblocks; ++j)
                        {
                            if (dirblocks.entries[j].blockID == dir_block_number[i])
                            {
                                if (j < (dirblocks.nentries - 1))
                                {
                                    dirblocks.entries[j] = dirblocks.entries[dirblocks.nentries - 1];
                                }
                                else
                                {
                                    dirblocks.entries[j] = dirblocks.entries[dirblocks.nentries];
                                }
                            }
                        }
                        // UPDATE NUMBER OF ENTRIES IN ROOT
                        dirblocks.nentries -= 1;
                        fseek(fp, root_location, SEEK_SET);
                        fwrite(&dirblocks, sizeof(dirblocks), 1, fp);
                    }

                    // UPDATE SUBDIRECTORY
                    if (t > 1)
                    {
                        int subdir_location = sizeof(hd) + sizeof(btmp) + (blocksize * subdir_block_id[s - 1]);
                        fseek(fp, subdir_location, SEEK_SET);
                        fread(&dirblocks, sizeof(dirblocks), 1, fp);
                        for (int j = 0; j < dirblocks.nentries; ++j)
                        {
                            if (dirblocks.entries[j].blockID == dir_block_number[i])
                            {
                                if (j < (dirblocks.nentries - 1))
                                {
                                    dirblocks.entries[j] = dirblocks.entries[dirblocks.nentries - 1];
                                }
                                else
                                {
                                    dirblocks.entries[j] = dirblocks.entries[dirblocks.nentries];
                                }
                            }
                        }
                        dirblocks.nentries -= 1;
                        fseek(fp, subdir_location, SEEK_SET);
                        fwrite(&dirblocks, sizeof(dirblocks), 1, fp);
                    }
                }
            }
        }
        if (!dirname_exists)
        {

            SIFS_errno = SIFS_ENOENT;
            return 1;
        }

        fclose(fp);
        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
