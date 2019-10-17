#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"
//  Written by Minh Smith 20956909 October 2019

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
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
            for (int i = 0; i < nfile; ++i)
            {
                int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * file_block_number[i]);
                fseek(fp, file_location, SEEK_SET);
                fread(&fileblock, sizeof(fileblock), 1, fp);
                for (int j = 0; j < SIFS_MAX_ENTRIES; ++j)
                {

                    if (strcmp(fileblock.filenames[j], path_tokens[t - 1]) == 0)
                    {
                        SIFS_errno = SIFS_ENOTDIR;
                        return 1;
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

        // FIND THE BLOCK NUMBER OF ALL THE DIRECTORIES 
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
        if (path_valid_count <(t-2))
        {
            SIFS_errno = SIFS_EINVAL;
            return 1;
        }

        // FIND THE DIRECTORY
        int directory_block_number = 0;
        int nentries_in_dir = 0;
        bool directory_found = false;
        for (int i = 0; i < ndir; ++i)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_numbers[i]);
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            if (strcmp(dirblocks.name, path_tokens[t - 1]) == 0)
            {
                 // CHECK IF PATH IS VALID PART TWO
                int block_id_in_search = dir_block_numbers[i];
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
                directory_block_number = dir_block_numbers[i];
                directory_found = true;
                
                *nentries = dirblocks.nentries;
                nentries_in_dir = dirblocks.nentries;
                *modtime = dirblocks.modtime;
            }
        }
        if (!directory_found)
        {
            SIFS_errno = SIFS_ENOENT;
        }


        // FIND THE BLOCK NUMBER OF ALL THE ENTRIES
        char **entries = NULL;
        entries = (char**)malloc(sizeof(char));
        if(entries == NULL)
        {
            SIFS_errno = SIFS_ENOMEM;
            return 1;
        }
        int size = nentries_in_dir;
        int nentries_in_block = 0;
        int dirblock_id_of_entries[size];
        int directory_location = sizeof(hd) + sizeof(btmp) + (blocksize * directory_block_number);
        
        fseek(fp, directory_location, SEEK_SET);
        fread(&dirblocks, sizeof(dirblocks), 1, fp);

        for (int j = 0; j < size; ++j)
        {
            dirblock_id_of_entries[j] = dirblocks.entries[j].blockID;
        }

        // LOOP THROUGH ALL THE BLOCKS TO FIND EITHER DIRECTORY NAMES OR FILENAMES
        for (int i = 0; i < size; i++)
        {
            if (btmp[dirblock_id_of_entries[i]] == SIFS_DIR)
            {
                entries = realloc(entries, (nentries_in_block + 1) * sizeof(entries[0]));
                int location = sizeof(hd) + sizeof(btmp) + (blocksize * dirblock_id_of_entries[i]);
                fseek(fp, location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                entries[nentries_in_block] = strdup(dirblocks.name);
                nentries_in_block++;
            }
            else if (btmp[dirblock_id_of_entries[i]] == SIFS_FILE)
            {
                entries = realloc(entries, (nentries_in_block + 1) * sizeof(entries[0]));
                int location = sizeof(hd) + sizeof(btmp) + (blocksize * dirblock_id_of_entries[i]);
                fseek(fp, location, SEEK_SET);
                fread(&fileblock, sizeof(fileblock), 1, fp);
                int fileblock_nentries = fileblock.nfiles;
                for (int j = 0; j < fileblock_nentries; ++j)
                {
                    entries = realloc(entries, (nentries_in_block + 1) * sizeof(entries[0]));
                    entries[nentries_in_block] = strdup(fileblock.filenames[j]);
                    nentries_in_block++;
                }
            }
        }

        *entrynames = entries;

        return 0;
    }
    else
    {
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
}
