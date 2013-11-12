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
    int entryCount;
    int entrySize;
    struct FileEntry *entries;
};

struct FileEntry {
    char path[1000];
    char hash[41];
    bool isNew;
};

void makeDirOrFail(const char *path);
void executeNextBlock(FILE *fd);
void initBackup(struct Backup *backup, int isFull);
void saveFileEntry(struct Backup *Backup, struct FileEntry *entry);
void freeBackup(struct Backup *backup);

#endif
