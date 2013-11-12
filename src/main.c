#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/sha.h>
#include "shared.h"

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
    backup->entryCount = 0;
    backup->entrySize = 0;
    backup->entries = 0;
}

void getBackupPaths(const char *argv, struct Backup *backup) {
    backup->dir = dirname(strdup(argv));
    backup->file = basename(strdup(argv));
    snprintf(backup->rootDir, sizeof(backup->rootDir) - 1, "%s.dat", backup->file);
    snprintf(backup->mirrorDir, sizeof(backup->mirrorDir) - 1, "%s/mirror", backup->rootDir);
    snprintf(backup->newDir, sizeof(backup->newDir) - 1, "%s/new", backup->rootDir);
}

void prepare(struct Backup *backup) {
    if (chdir(backup->dir)) {
        perror("Failed to change to data directory");
        exit(1);
    }
   
    if ((backup->fd = fopen(backup->file, "r")) == 0) {
        perror("Failed to open backup file descriptor");
        exit(1);
    }

    makeDirOrFail(backup->rootDir);
    makeDirOrFail(backup->mirrorDir);
    makeDirOrFail(backup->newDir);
}

void performMirror(struct Backup *backup) {
    if (chdir(backup->mirrorDir)) {
        perror("Failed to chdir mirror");
        exit(1);
    }

    executeNextBlock(backup->fd);

    if (chdir("../../")) {
        perror("Failed to chdir data");
        exit(1);
    }
}

void hashFile(const char *path, char *shaHash) {
    char buf[4096];
    FILE *fd = fopen(path, "rb");
    size_t read;
    SHA_CTX shaCtx;
    SHA1_Init(&shaCtx);
    while ((read = fread(buf, 1, sizeof(buf), fd))) {
        SHA1_Update(&shaCtx, buf, read);
    }
    fclose(fd);
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &shaCtx);
    
    for (int offset = 0; offset < SHA_DIGEST_LENGTH; ++offset) {
        sprintf(shaHash + offset * 2, "%02x", hash[offset]);
    }
    shaHash[SHA_DIGEST_LENGTH * 2] = 0;
}

void hashFiles(const char *path, struct Backup *backup) {
    DIR *dir;
    struct dirent *entry;
    char hashedFile[1024];
    struct stat entryStat;

    if ((dir = opendir(path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                struct FileEntry fileEntry;
                int len = snprintf(fileEntry.path, sizeof(fileEntry.path) - 1, "%s/%s", path, entry->d_name);
                fileEntry.path[len] = 0;

                stat(fileEntry.path, &entryStat);
                if (S_ISDIR(entryStat.st_mode)) {
                    hashFiles(fileEntry.path, backup);
                } else {
                    hashFile(fileEntry.path, fileEntry.hash);
                    len = snprintf(hashedFile, sizeof(hashedFile) - 1, "%s/%s", backup->newDir, fileEntry.hash);
                    hashedFile[len] = 0;

                    fileEntry.isNew = access(hashedFile, F_OK);
                    saveFileEntry(backup, &fileEntry);

                    if (fileEntry.isNew) {
                        link(fileEntry.path, hashedFile);
                    }
                }
            }
        }
        closedir(dir);
    }
}

void hashMirrored(struct Backup *backup) {
    hashFiles(backup->mirrorDir, backup);
}

int main(int argc, const char **argv) {
    struct Backup backup;

    if (argc != 3) {
        printUsage();
    }

    getAction(argv[1], &backup);
    getBackupPaths(argv[2], &backup);

    printf("Action: %s\n", backup.isFull ? "full" : "incremental");
    printf("Dir   : %s\n", backup.dir);
    printf("File  : %s\n", backup.file);
    printf("Root  : %s\n", backup.rootDir);
    printf("Mirror: %s\n", backup.mirrorDir);
    printf("New   : %s\n", backup.newDir);

    prepare(&backup);
    performMirror(&backup);
    hashMirrored(&backup);

    fclose(backup.fd);

    for (int i = 0; i < backup.entryCount; ++i) {
        printf("%s : %s%s\n",
            backup.entries[i].hash,
            backup.entries[i].isNew ? "*" : "",
            backup.entries[i].path);
    }

    freeBackup(&backup);

    printf("bye\n");
    return 0;
}
