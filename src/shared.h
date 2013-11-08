#ifndef _SHARED_H_
#define _SHARED_H_

struct Backup {
    bool isFull;
    char *dir;
    char *file;
    char *rootDir;
    char *mirrorDir;
    char *newDir;
    FILE *fd;
};

void makeDirOrFail(const char *path);
void executeNextBlock(FILE *fd);

#endif
