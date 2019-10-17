#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"

//  Written by Minh Smith 20956909 October 2019

// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
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

        // CHECK IF PATH IS VALID PART ONE
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
        int block_number_of_file = 0;
        int first_data_block = 0;
        int data_size = 0;
        int file_entry_number = 0;
        int nfiles_in_fileblock = 0;
        bool file_found = false;

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
                    // UPDATE ENTRY AND FILES
                    block_number_of_file = file_block_number[i];
                    first_data_block = fileblock.firstblockID;
                    data_size = fileblock.length;
                    file_entry_number = j;
                    nfiles_in_fileblock = fileblock.nfiles;
                    file_found = true;

                    // CHECK IF PATH IS VALID PART TWO
                    int block_id_in_search = block_number_of_file;
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

                    if (nreverse != (t-1))
                    {
                        SIFS_errno = SIFS_EINVAL;
                        return 1;
                    }

                    if (j < (nfiles_in_fileblock - 1))
                    {
                        strcpy(fileblock.filenames[j], fileblock.filenames[nfiles_in_fileblock - 1]);
                    }
                    else
                    {
                        strcpy(fileblock.filenames[j], fileblock.filenames[nfiles_in_fileblock]);
                    }
                    fileblock.nfiles--;
                    fseek(fp, file_location, SEEK_SET);
                    fwrite(&fileblock, sizeof(fileblock), 1, fp);
                    break;
                }
            }
        }

        if (!file_found)
        {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }

        // IF THERE IS ONLY ONE ENTRY IN FILEBLOCK
        char oneblock[blocksize];
        if (nfiles_in_fileblock == 1)
        {
            // write over file block
            int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * block_number_of_file);
            fseek(fp, file_location, SEEK_SET);
            fwrite(oneblock, sizeof(oneblock), 1, fp);
            btmp[block_number_of_file] = SIFS_UNUSED;

            int number_of_data_blocks = (data_size / blocksize) + 1;
            int data_block_location = sizeof(hd) + sizeof(btmp) + (blocksize * first_data_block);
            fseek(fp, data_block_location, SEEK_SET);
            // write over file
            for (int i = 0; i < number_of_data_blocks; i++)
            {
                fwrite(oneblock, sizeof(oneblock), 1, fp);
            }
            // set all datablocks in bitmap to "unused"
            for (int b = first_data_block; b < (first_data_block + number_of_data_blocks); ++b)
            {
                btmp[b] = SIFS_UNUSED;
            }
            fseek(fp, sizeof(hd), SEEK_SET);
            fwrite(btmp, sizeof(btmp), 1, fp);
        }

        // UPDATE THE ROOT IF THERE WAS NO SUBDIRECTORIES
        if (t == 1)
        {
            int root_location = sizeof(hd) + sizeof(btmp);
            fseek(fp, root_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            // FIND THE ENTRY NUMBER
            for (int j = 0; j < SIFS_MAX_ENTRIES; ++j)
            {
                if (dirblocks.entries[j].blockID == block_number_of_file)
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
                if (dirblocks.entries[j].blockID == block_number_of_file)
                {
                    if (j < (dirblocks.nentries - 1))
                    {
                        dirblocks.entries[j] = dirblocks.entries[dirblocks.nentries - 1];
                        dirblocks.entries[j].fileindex--;
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

        fclose(fp);
        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
