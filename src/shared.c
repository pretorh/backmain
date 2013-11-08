#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "shared.h"

void makeDirOrFail(const char *path) {
    if (access(path, F_OK) && (mkdir(path, S_IRWXU))) {
        printf("Failed to create directory %s: %s\n", path, strerror(errno));
        exit(1);
    }
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
