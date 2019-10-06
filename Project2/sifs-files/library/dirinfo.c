#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>


#include "sifs-internal.h"


// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
    //  ENSURE THAT RECEIVED PARAMETERS ARE VALID
    if (volumename == NULL || pathname == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }


    SIFS_DIRBLOCK db;
    SIFS_FILEBLOCK sfb;
    FILE *fp;
    
    // ATTEMPT TO OPEN FILE FOR READING
    fp = fopen(volumename, "r");

    // IF FILE CANNOT OPEN
    if(fp == NULL)
    {
        SIFS_errno = SIFS_ENOENT;
    }
    else 
    {
        // GET NUMBER OF ENTRIES AND MODTIME
        fread(&db, sizeof(SIFS_DIRBLOCK), 1, fp);
        nentries = &db.nentries;
        modtime = &db.modtime;
        fseek(fp, 0, SEEK_SET);

        // GET BLOCK NUMBER FOR ENTRIES
        fread(&sfb, sizeof(SIFS_FILEBLOCK), 1, fp);
        for (int i=0; i<(int)nentries; ++i) {
            int blockID = db.entries[i].blockID;
            **entrynames = strdup(sfb.filenames[blockID]);
            printf("%s\n", **entrynames);
        }

        // TODO: ADD ENTRIES TO VOLUME TO CHECK THIS SECTION
    
    }

    fclose(fp);

    return 1;

}

