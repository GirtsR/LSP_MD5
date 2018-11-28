#include <stdio.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MY_BUFFER_SIZE 1024
#define BLOCK_SIZE 16
#define BLOCK_COUNT MY_BUFFER_SIZE / BLOCK_SIZE

unsigned char mybuffer[MY_BUFFER_SIZE];

struct Segment *root;
struct Segment *lastFit;
size_t lastFitCount;

struct Segment {
    struct Segment *prev;
    struct Segment *next;
    size_t blockCount;// Multiply by BLOCK_SIZE to get size in bytes
    char used;
};

void *NextFit(size_t size) {
    if (size == 0) {
        return NULL;
    }

    struct Segment *cur = lastFit;
    size_t curCount = lastFitCount;// Avoid recount
    size_t neededCount = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    do {
        if (!cur->used && neededCount <= cur->blockCount) {

            if (neededCount < cur->blockCount) {
                // Place new empty segment between current and next segment
                struct Segment *split = malloc(sizeof(struct Segment));// Shhhh

                split->prev = cur;
                split->next = cur->next;
                split->blockCount = cur->blockCount - neededCount;
                split->used = 0;

                if (cur->next) {
                    cur->next->prev = split;
                }
                cur->next = split;
            }

            cur->used = 1;
            cur->blockCount = neededCount;

            lastFit = cur;
            lastFitCount = curCount;

            return mybuffer + curCount * BLOCK_SIZE;
        }

        if (cur->next) {
            // Go forward
            curCount += cur->blockCount;
            cur = cur->next;
        } else {
            // Loop around
            curCount = 0;
            cur = root;
        }
    } while (cur != lastFit);

    return NULL;
}

void printMemory() {
    struct Segment *cur = root;

    while (cur) {
        printf("|");
        for (int i = 0; i < cur->blockCount - 1; i++) {
            printf(cur->used ? "_" : ".");
        }
        cur = cur->next;
    }
    printf("|\n");
}

void setup() {
    root = malloc(sizeof(struct Segment));
    root->prev = NULL;
    root->next = NULL;
    root->blockCount = BLOCK_COUNT;
    root->used = 0;
    lastFit = root;
    lastFitCount = 0;
}

int main(int argc, char **argv) {
    bool chunks_specified = false;
    bool sizes_specified = false;
    char *chunk_file = NULL;
    char *size_file = NULL;
    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            chunk_file = argv[++i];
            chunks_specified = true;
        } else if (strcmp(argv[i], "-s") == 0) {
            size_file = argv[++i];
            sizes_specified = true;
        }
    }
    if (!chunks_specified || !sizes_specified) {
        fprintf(stderr, "Argument -c or -s not specified\n");
        return -1;
    }

    //TODO - set up chunks from file
    setup();

    //TODO - read allocations from file
    void *a = NextFit(128);
    void *b = NextFit(256);
    void *c = NextFit(512);

    printMemory();

    void *d = NextFit(64);

    printMemory();

    void *e = NextFit(128);

    printMemory();

    return 0;

    return 0;
}