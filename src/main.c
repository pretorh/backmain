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

struct Backup {
    bool isFull;
    char *dir;
    char *file;
    char *rootDir;
    char *mirrorDir;
    char *newDir;
    FILE *fd;
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

void prepare(struct Backup *backup) {
    if (chdir(backup->dir)) {
        printf("Failed to change to data directory: %s\n", strerror(errno));
        exit(1);
    }
   
    if ((backup->fd = fopen(backup->file, "r")) == 0) {
        printf("Failed to open backup file descriptor: %s\n", strerror(errno));
        exit(1);
    }

    makeDirOrFail(backup->rootDir);
    makeDirOrFail(backup->mirrorDir);
    makeDirOrFail(backup->newDir);
}

void executeNextBlock(FILE *fd) {
    char line[1000];
    bool done, isComment = false;
    while (!done && fgets(line, 1000, fd)) {
        done = !strcmp(line, "\n");
        isComment = strlen(line) >= 1 && line[0] == '#';

        if (!done && !isComment) {
            printf("%s\n", line);
            system(line);
        }
    }
}

void performMirror(struct Backup *backup) {
    if (chdir(backup->mirrorDir)) {
        printf("Failed to chdir mirror: %s\n", strerror(errno));
        exit(1);
    }

    executeNextBlock(backup->fd);

    if (chdir("../../")) {
        printf("Failed to chdir data: %s\n", strerror(errno));
        exit(1);
    }
}

void hashFile(const char *path) {
    printf("%s\n", path);
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
    
    char shaHash[SHA_DIGEST_LENGTH * 2 + 1];
    for (int offset = 0; offset < SHA_DIGEST_LENGTH; ++offset) {
        sprintf(shaHash + offset * 2, "%02x", hash[offset]);
    }
    shaHash[SHA_DIGEST_LENGTH * 2] = 0;

    printf("%s\n", shaHash);
}

void hashFiles(const char *path) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                char relative[1024];
                int len = snprintf(relative, sizeof(relative) - 1, "%s/%s", path, entry->d_name);
                relative[len] = 0;

                struct stat entryStat;
                stat(relative, &entryStat);
                if (S_ISDIR(entryStat.st_mode)) {
                    hashFiles(relative);
                } else {
                    hashFile(relative);
                }
            }
        }
        closedir(dir);
    }
}

void hashMirrored(struct Backup *backup) {
    hashFiles(backup->mirrorDir);
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

    prepare(&backup);
    performMirror(&backup);
    hashMirrored(&backup);

    fclose(backup.fd);

    printf("bye\n");
    return 0;
}
