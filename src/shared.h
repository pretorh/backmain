#ifndef _SHARED_H_
#define _SHARED_H_

struct Backup {
    bool isFull;
    char *dir;
    char *file;
    char rootDir[1000];
    char mirrorDir[1000];
    char newDir[1000];
    FILE *fd;
};

void makeDirOrFail(const char *path);
void executeNextBlock(FILE *fd);

#endif
