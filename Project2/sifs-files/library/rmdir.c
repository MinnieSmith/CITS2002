#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>

#include "sifs-internal.h"

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
        printf("%i, %zu \n", nblocks, hd.blocksize);

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
            for (int b = 1; b < nblocks; ++b)
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
        printf("Bitmap: %s\n", btmp);

        for (int b = 0; b < nblocks; ++b)
        {
            if (btmp[b] == SIFS_DIR)
            {
                dir_block_number[n] = b;
                printf("Directory block number: %i\n", dir_block_number[n]);
                n++;
            }
        }
        printf("number of directories (n) = %i\n", n);

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
                        printf("Subdir block location: %i\n", subdir_block_id[s]);
                        s++;
                    }
                    token++;
                } while (token < (t - 1));
            }
            printf("Number of subdir: %i\n", s);

            if (s < (t - 1))
            {
                SIFS_errno = SIFS_EINVAL;
                printf("No such subdirectory, please check pathname\n");
                return 1;
            }
        }

        // CHECK IF PATH IS VALID
        printf("194 CHECK IF PATH IS VALID\n\n");
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

        // CHECK IF DIRECTORY NAME EXISTS
        printf("219 CHECK IF DIRECTORY NAME EXISTS\n\n");
        bool dirname_exists = false;
        char oneblock[blocksize];
        for (int i = 0; i < ndir; i++)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            printf("dirblock name: %s\n", dirblocks.name);
            if (strcmp(dirblocks.name, path_tokens[t - 1]) == 0)
            {
                //CHECK IF DIRECTORY IS EMPTY REMOVE DIRECTORY AND UPDATE BITMAP
                dirname_exists = true;
                printf("230 CHECK IF DIRECTORY IS EMPTY REMOVE DIRECTORY AND UPDATE BITMAP\n\n");
                if (dirblocks.nentries == 0)
                {
                    printf("DIRBLOCK ENTRY = 0\n");
                    fseek(fp, dir_location, SEEK_SET);
                    memset(oneblock, 0, sizeof(oneblock));   // reset block to zero
                    btmp[dir_block_number[i]] = SIFS_UNUSED; // update bitmap
                    printf("Bitmap: %s\n", btmp);
                    fseek(fp, sizeof(hd), SEEK_SET);
                    fwrite(btmp, sizeof(btmp), 1, fp);

                    // UPDATE THE ROOT
                    if (t == 1)
                    {
                        printf("244 UPDATE THE ROOT\n\n");
                        int root_location = sizeof(hd) + sizeof(btmp);
                        fseek(fp, root_location, SEEK_SET);
                        fread(&dirblocks, sizeof(dirblocks), 1, fp);
                        // FIND THE ENTRY NUMBER
                        printf("249 FIND THE ENTRY NUMBER IN ROOT TO BE DELETED\n\n");
                        for (int j = 0; j < nblocks; ++j)
                        {
                            if (dirblocks.entries[j].blockID == dir_block_number[i])
                            {
                                printf("254 ENTRY LOCATION = %i\n", j);
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
                        printf("266 UPDATE NUMBER OF ENTRIES IN ROOT\n\n");
                        dirblocks.nentries -= 1;
                        fseek(fp, root_location, SEEK_SET);
                        fwrite(&dirblocks, sizeof(dirblocks), 1, fp);
                    }

                    // UPDATE SUBDIRECTORY
                    if (t > 1)
                    {
                        printf("275 UPDATE SUBDIRECTORY\n\n");
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
                    printf("Bitmap after dir removed: %s\n", btmp);
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
