#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "sifs-internal.h"

//  Written by Minh Smith 20956909 October 2019

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

        // FIND THE NUMBER OF BLOCKS IN VOLUME
        SIFS_VOLUME_HEADER hd;
        int nblocks;
        int blocksize;
        fread(&hd, sizeof(hd), 1, fp);
        nblocks = hd.nblocks;
        blocksize = hd.blocksize;

        // FIND THE NUMBER OF FREE BLOCKS
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

        // CHECK IF LAST TOKEN IS A FILE
        // FIND NUMBER OF FILES IN VOLUME
        int nfile = 0;
        for (int i = 0; i < nblocks; ++i)
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
                            SIFS_errno = SIFS_EEXIST;
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
            if (nreverse != (t-1))
            {
                SIFS_errno = SIFS_EINVAL;
                return 1;
            }
        }

        // CHECK IF FILENAME IS THE SAME AS DIRECTORY NAME
        for (int i = 1; i < ndir; ++i)
        {
            int dir_location = sizeof(hd) + sizeof(btmp) + (blocksize * dir_block_number[i]);
            fseek(fp, dir_location, SEEK_SET);
            fread(&dirblocks, sizeof(dirblocks), 1, fp);
            if (strcmp(dirblocks.name, path_tokens[t - 1]) == 0)
            {
                SIFS_errno = SIFS_EEXIST;
                return 1;
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------------------
        // CHECK FOR DUPLICATE FILES IN VOLUME
        //----------------------------------------------------------------------------------------------------------------------------------------

        // CALCULATE THE MD5 OF THE DATA
        unsigned char md5_infile[MD5_BYTELEN];
        MD5_buffer(data, nbytes, md5_infile);
        bool duplicate_md5 = false;

        int block_number_of_duplicate_md5 = 0;

        //  CHECK IF MD5 ALREADY EXISTS
        for (int i = 0; i < nfile; ++i)
        {
            int file_location = sizeof(hd) + sizeof(btmp) + (blocksize * file_block_number[i]);
            fseek(fp, file_location, SEEK_SET);
            fread(&fileblock, sizeof(fileblock), 1, fp);

            if (memcmp(md5_infile, fileblock.md5, sizeof(md5_infile)) == 0)
            {
                strcpy(fileblock.filenames[fileblock.nfiles], path_tokens[t - 1]);
                fileblock.nfiles++;
                duplicate_md5 = true;
                block_number_of_duplicate_md5 = file_block_number[i];

                fseek(fp, file_location, SEEK_SET);
                fwrite(&fileblock, sizeof(fileblock), 1, fp);
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------------------
        // IF MDU5 EXISTS
        //----------------------------------------------------------------------------------------------------------------------------------------
        if (duplicate_md5)
        {

            // UPDATE THE ROOT
            if (t == 1)
            {
                int root_location = sizeof(hd) + sizeof(btmp);
                fseek(fp, root_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                if (dirblocks.nentries == SIFS_MAX_ENTRIES)
                {
                    SIFS_errno = SIFS_EMAXENTRY;
                    return 1;
                }
                else
                {
                    dirblocks.entries[dirblocks.nentries].blockID = block_number_of_duplicate_md5;
                    dirblocks.entries[dirblocks.nentries].fileindex = fileblock.nfiles - 1;
                    dirblocks.nentries += 1;
                    fseek(fp, root_location, SEEK_SET);
                    fwrite(&dirblocks, sizeof(dirblocks), 1, fp);
                }
            }

            // UPDATE ENTRY ON SUBDIRECTORY
            if (t > 1)
            {
                SIFS_DIRBLOCK subdir_to_be_updated;
                int file_index = 0;
                int subdir_to_be_updated_location = sizeof(hd) + sizeof(btmp) + (blocksize * subdir_block_id[s - 1]);
                fseek(fp, subdir_to_be_updated_location, SEEK_SET);
                fread(&subdir_to_be_updated, sizeof(subdir_to_be_updated), 1, fp);

                if (subdir_to_be_updated.nentries == SIFS_MAX_ENTRIES)
                {
                    SIFS_errno = SIFS_EMAXENTRY;
                    return 1;
                }
                else
                {
                    for (int i = 0; i < SIFS_MAX_ENTRIES; ++i)
                    {
                        if (subdir_to_be_updated.entries[i].blockID == block_number_of_duplicate_md5)
                        {
                            file_index = subdir_to_be_updated.entries[i].fileindex;
                        }
                    }
                    subdir_to_be_updated.entries[subdir_to_be_updated.nentries].blockID = block_number_of_duplicate_md5;
                    subdir_to_be_updated.entries[subdir_to_be_updated.nentries].fileindex = (file_index + 1);
                    subdir_to_be_updated.nentries += 1; // update number of entries
                    fseek(fp, subdir_to_be_updated_location, SEEK_SET);
                    fwrite(&subdir_to_be_updated, sizeof(subdir_to_be_updated), 1, fp);
                    return 0;
                }
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------------------
        // IF THERE ARE NO DUPLICATE FILES ALREADY IN VOLUME
        //----------------------------------------------------------------------------------------------------------------------------------------

        if (!duplicate_md5)
        {
            // CALCULATE THE NUMBER OF BLOCKS REQURIED FOR DATA
            int data_blocks_required = (nbytes / blocksize) + 1;
            int total_blocks_required = data_blocks_required + 1;
            int free_block_count = 0;
            for (int b = 1; b < nblocks; ++b)
            {
                if (btmp[b] == SIFS_UNUSED)
                {
                    free_block_count++;
                }
            }

            if (total_blocks_required > free_block_count)
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
                        free_contiguous_block_number = b - (data_blocks_required - 1);
                        enough_datablocks = true;
                        break;
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
                return 1;
            }

            // WRITE DATA TO DATABLOCKS
            int data_block_location = sizeof(hd) + sizeof(btmp) + (blocksize * free_contiguous_block_number);
            fseek(fp, data_block_location, SEEK_SET);
            for (int i = 0; i < data_blocks_required; ++i)
            {
                fwrite((char *)data + (i * blocksize), blocksize, 1, fp);
                btmp[free_contiguous_block_number + i] = SIFS_DATABLOCK;
            }

            // FIND INDEX OF FREE BLOCK FOR FILEBLOCK ASSIGNMENT
            int free_block_index = 0;
            for (int b = 1; b < nblocks; ++b)
            {

                if (btmp[b] == SIFS_UNUSED)
                {
                    free_block_index = b;
                    break;
                }
            }

            // CREATE FILEBLOCK AND UPDATE VOLUME NEW FILEBLOCK
            SIFS_FILEBLOCK newfile;
            strcpy(newfile.filenames[0], path_tokens[t - 1]);
            newfile.firstblockID = free_contiguous_block_number;
            // // char *md5_formatted = MD5_format(md5_infile);
            memcpy(newfile.md5, md5_infile, sizeof(md5_infile));
            newfile.length = nbytes;
            newfile.nfiles = 1;
            newfile.modtime = time(NULL);

            int newfile_location = sizeof(hd) + sizeof(btmp) + (free_block_index * blocksize);
            fseek(fp, newfile_location, SEEK_SET);
            fwrite(&newfile, blocksize, 1, fp);
            btmp[free_block_index] = SIFS_FILE;

            // UPDATE BITMAP
            fseek(fp, sizeof(hd), SEEK_SET);
            fwrite(btmp, sizeof(btmp), 1, fp);

            // UPDATE THE ROOT
            if (t == 1)
            {
                int root_location = sizeof(hd) + sizeof(btmp);
                fseek(fp, root_location, SEEK_SET);
                fread(&dirblocks, sizeof(dirblocks), 1, fp);
                dirblocks.entries[dirblocks.nentries].blockID = free_block_index;
                dirblocks.entries[dirblocks.nentries].fileindex = 0;
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
                subdir_to_be_updated.entries[subdir_to_be_updated.nentries].fileindex = 0;
                subdir_to_be_updated.nentries += 1; // update number of entries
                fseek(fp, subdir_to_be_updated_location, SEEK_SET);
                fwrite(&subdir_to_be_updated, sizeof(subdir_to_be_updated), 1, fp);
            }
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
