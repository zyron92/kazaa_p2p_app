#include "file_handler.h"

/*
 * To return the size in bytes of the read file
 */
static long readFileSize(const char *filename)
{
    FILE *new_file = fopen(filename, "rb");
    if (new_file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file\n");
        exit(1);
    }

    //Looking for the size of file
    fseek(new_file, 0, SEEK_END);
    long file_size = ftell(new_file);
    fseek(new_file, 0, SEEK_SET);

    fclose(new_file);
    return file_size;
}

/*
 * Filter function for scandir :
 */
static int filesToSelect(const struct dirent *entry)
{
    if (entry == NULL || entry->d_name == NULL) {
        return 0;
    }

    //skip the directories (. & ..)
    if ((strlen(entry->d_name) == 1) && (strncmp(entry->d_name, ".", 1) == 0)) {
        return 0;
    }

    if ((strlen(entry->d_name) == 2) &&
        (strncmp(entry->d_name, "..", 1) == 0)) {
        return 0;
    }

    return 1;
}

/*
 * Scan folder for files in the directory of data/ or other directories
 */
HashTableElem2 *scan_folder(const char *foldername, int *numFiles)
{
    struct dirent **filename_list;
    *numFiles = scandir(foldername, &filename_list, filesToSelect,
                        alphasort);
    if (*numFiles < 1) {
        fprintf(stderr, "ERROR: scan directory failed\n");
        return NULL;
    } else {
        //Create a list of files to store information on files
        HashTableElem2 *files_list = calloc(*numFiles, sizeof(HashTableElem2));
        if (files_list == NULL) {
            fprintf(stderr, "ERROR: Allocation of memory failed\n");
            return NULL;
        }

        int sizeFilename;
        int sizeFoldername;
        char *latestFilename;
        int i;
        for (i = 0; i < *numFiles; i++) {
            //Create a new filename with a "folderName/", eg: data/ so that we can read the file
            sizeFilename = strlen(filename_list[i]->d_name);
            sizeFoldername = strlen(foldername);
            latestFilename = calloc(sizeFoldername + 1 + sizeFilename + 1,
                                    sizeof(char));
            strncpy(latestFilename, foldername, sizeFoldername);
            strncpy(latestFilename + sizeFoldername, "/", 1);
            strncpy(latestFilename + sizeFoldername + 1,
                    filename_list[i]->d_name, sizeFilename);
            latestFilename[sizeFoldername + 1 + sizeFilename] = '\0';

            //store the informations
            //by finding the file size and store the filesize for each file
            //and converting string file name to integer file index and store it for each file
            //without the directory's name
            files_list[i].file_size = (unsigned int) (readFileSize(
                    latestFilename));
            files_list[i].file_index = calloc(
                    strlen(filename_list[i]->d_name) + 1, sizeof(char));
            strncpy(files_list[i].file_index, filename_list[i]->d_name,
                    strlen(filename_list[i]->d_name));

            free(latestFilename);
            latestFilename = NULL;
            free(filename_list[i]);
        }
        free(filename_list);

        return files_list;
    }
}

/*
 * Save data in a file in directory download/
 */
int save_data(Packet *readPacket, const char *filename)
{
    int filesize = (readPacket->totalLength) - 12;

    //Create a new filename with a prefix download/
    char latestFilename[strlen(filename) + 9 + 1];
    strncpy(latestFilename, "download/", 9);
    strncpy(latestFilename + 9, filename, strlen(filename));
    latestFilename[strlen(filename) + 9] = '\0';

    //Open and write the whole received data
    FILE *new_file = fopen(latestFilename, "wb");
    fwrite(readPacket->info, sizeof(char), filesize, new_file);
    fclose(new_file);
    return OK;
}

/*
 * Reading the whole file from data/ and send them as strings
 */
char *read_whole_file(const char *filename, int *filesize)
{
    *filesize = 0;

    //Create a new filename with a prefix data/
    char latestFilename[strlen(filename) + 5 + 1];
    strncpy(latestFilename, "data/", 5);
    strncpy(latestFilename + 5, filename, strlen(filename));
    latestFilename[strlen(filename) + 5] = '\0';

    FILE *new_file = fopen(latestFilename, "rb");
    if (new_file == NULL) {
        fprintf(stderr, "ERROR: Failed to open file\n");
        return NULL;
    }

    //Looking for the size of file
    fseek(new_file, 0, SEEK_END);
    long file_size = ftell(new_file);
    *filesize = file_size;
    fseek(new_file, 0, SEEK_SET);

    //Reading the whole file and put them onto a string
    char *output_string;
    if (file_size > 0) {
        output_string = calloc(file_size, sizeof(char));
        fread(output_string, file_size, 1, new_file);
    } else {
        *filesize = 0;
        output_string = NULL;
    }

    fclose(new_file);

    return output_string;
}