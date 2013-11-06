#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

struct Backup {
    bool isFull;
    char *dir;
    char *file;
    char *rootDir;
    char *mirrorDir;
    char *newDir;
};

void printUsage() {
    printf("Invalid arguments:\n");
    printf("\taction [f|full|i|incremental]\n");
    printf("\tbackup descriptor file\n");
    exit(1);
}

void getAction(const char *argv, struct Backup *backup) {
    bool isFull = strcmp(argv, "f") == 0 || strcmp(argv, "full") == 0;
    bool isIncremental = strcmp(argv, "i") == 0 || strcmp(argv, "incremental") == 0;
    if (!isFull && !isIncremental) {
        printUsage();
    }
    backup->isFull = isFull;
}

void getDescriptorFile(const char *argv, struct Backup *backup) {
    backup->dir = dirname(strdup(argv));
    backup->file = basename(strdup(argv));
    
    backup->rootDir = strdup(backup->file);
    strcat(backup->rootDir, ".dat/");
    
    backup->mirrorDir = strdup(backup->rootDir);
    strcat(backup->mirrorDir, "mirror/");

    backup->newDir = strdup(backup->rootDir);
    strcat(backup->newDir, "new/");
}

void makeDirOrFail(const char *path) {
    if (access(path, F_OK) && (mkdir(path, S_IRWXU))) {
        printf("Failed to create directory %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

void setupDataDir(struct Backup *backup) {
    if (chdir(backup->dir)) {
        printf("Failed to change to data directory: %s\n", strerror(errno));
        exit(1);
    }
   
    makeDirOrFail(backup->rootDir);
    makeDirOrFail(backup->mirrorDir);
    makeDirOrFail(backup->newDir);
}

int main(int argc, const char **argv) {
    struct Backup backup;

    if (argc != 3) {
        printUsage();
    }

    getAction(argv[1], &backup);
    getDescriptorFile(argv[2], &backup);

    printf("Action: %s\n", backup.isFull ? "full" : "incremental");
    printf("Dir   : %s\n", backup.dir);
    printf("File  : %s\n", backup.file);
    printf("Root  : %s\n", backup.rootDir);
    printf("Mirror: %s\n", backup.mirrorDir);
    printf("New   : %s\n", backup.newDir);

    setupDataDir(&backup);

    printf("bye\n");
    return 0;
}
