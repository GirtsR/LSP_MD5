#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MY_BUFFER_SIZE 1024
#define DEBUG 0 /* 0 - do not include debug code, 1 - include debug code */

unsigned char mybuffer[MY_BUFFER_SIZE];

struct Segment *root;
struct Segment *lastFit;

struct Segment {
    struct Segment *prev;
    struct Segment *next;
    void *address;
    size_t size;
    char used;
};

void *allocate_from_block(struct Segment **segment, size_t size) {
    if ((*segment)->size < size) {
        #if DEBUG
        fprintf(stderr, "Error allocating block of size %zu\n", size);
        #endif
        return NULL;
    }

    if (size < (*segment)->size) {
        /* Place new empty segment between current and next segment */
        struct Segment *split = malloc(sizeof *split);

        split->prev = (*segment);
        split->next = (*segment)->next;
        split->size = (*segment)->size - size;
        split->used = 0;
        split->address = (*segment)->address + size;

        if ((*segment)->next) {
            (*segment)->next->prev = split;
        }
        (*segment)->next = split;
    }

    (*segment)->size = size;
    (*segment)->used = 1;
    return (*segment)->address;
}

void *WorstFit(size_t size) {
    if (size == 0) {
        return NULL;
    }

    struct Segment *current = root;

    size_t max_size = 0;
    struct Segment *max_segment = NULL;

    while (current != NULL) {
        if (current->size > max_size && !current->used) {
            max_segment = current;
            max_size = current->size;
        }
        current = current->next;
    }

    if (max_segment == NULL) {
        return NULL;
    } else {
        return allocate_from_block(&max_segment, size);
    }
}

void *BestFit(size_t size) {
    if (size == 0) {
        return NULL;
    }

    struct Segment *current = root;

    size_t min_size_diff = ~(size_t) 0;
    struct Segment *best_segment = NULL;

    while (current != NULL) {
        if ((current->size >= size) && (current->size - size < min_size_diff) && !current->used) {
            best_segment = current;
            min_size_diff = current->size - size;
        }
        current = current->next;
    }

    if (best_segment == NULL) {
        return NULL;
    } else {
        return allocate_from_block(&best_segment, size);
    }
}

void *NextFit(size_t size) {
    if (size == 0) {
        return NULL;
    }

    struct Segment *cur = lastFit;

    do {
        if (!cur->used && size <= cur->size) {
            void *address = allocate_from_block(&cur, size);
            lastFit = cur;
            return address;
        }

        if (cur->next) {
            /* Go forward */
            cur = cur->next;
        } else {
            /* Loop around */
            cur = root;
        }
    } while (cur != lastFit);

    return NULL;
}

void *FirstFit(size_t size) {
    struct Segment *tmp_node = root;
    while (tmp_node && (tmp_node->used || tmp_node->size < size)) {
        tmp_node = tmp_node->next;
    }

    if (!tmp_node) {
        return NULL;
    }

    return allocate_from_block(&tmp_node, size);
}

void printMemory() {
    struct Segment *cur = root;

    while (cur) {
        printf("|");
        int i;
        for (i = 0; i < (int) cur->size - 1; i++) {
            printf(cur->used ? "_" : ".");
        }
        cur = cur->next;
    }
    printf("|\n");
}

void printChunks() {
    struct Segment *current = root;

    while (current != NULL) {
        printf("Chunk: %zu, address: %zu\n", current->size, current->address - (void *) mybuffer);
        current = current->next;
    }
}

struct Segment *create_node(const size_t size) {
    struct Segment *node = malloc(sizeof(struct Segment));
    node->prev = NULL;
    node->next = NULL;
    node->size = size;
    node->used = 0;
    node->address = mybuffer;

    return node;
}

struct Segment *add_block(const size_t size) {
    struct Segment *current = create_node(size);
    struct Segment *previous = root;
    if (root == NULL) {
        root = current;
    } else {
        while (previous->next != NULL) {
            previous = previous->next;
        }
        previous->next = current;
        current->prev = previous;

        current->address = previous->address + previous->size;
    }
    return current;
}

struct SizeNode {
    struct SizeNode *next;
    size_t size;
    float fragmentation; /* After allocation */
    char failed;
} sizeNode;

struct SizeNode *create_size_node(size_t size) {
    struct SizeNode *tmp = calloc(1, sizeof sizeNode);
    tmp->size = size;
    tmp->next = NULL;
    tmp->failed = 0;
    return tmp;
}

void list_add_size(struct SizeNode **head, struct SizeNode **tail, size_t size) {
    if (*head == NULL) {
        *head = *tail = create_size_node(size);
        return;
    }

    (*tail)->next = create_size_node(size);
    (*tail) = (*tail)->next;
}

struct SizeNode *read_request_sizes(const char *size_file) {
    FILE *size_input = fopen(size_file, "r");
    size_t size = 0;

    struct SizeNode *head = NULL;
    struct SizeNode *tail = NULL;

    while (fscanf(size_input, "%zu", &size) != EOF) {
        list_add_size(&head, &tail, size);
    }

    fclose(size_input);

    return head;
}

void print_sizes(struct SizeNode *sizes_head) {
    printf("--------- Sizes start ----------\n");
    while (sizes_head) {
        printf("%zu\n", sizes_head->size);
        sizes_head = sizes_head->next;
    }
    printf("--------- Sizes end -------------\n");
}

void read_chunks(const char *chunk_file) {
    FILE *chunk_input = fopen(chunk_file, "r");

    size_t cur_size = 0;
    fscanf(chunk_input, "%zu", &cur_size);

    while (!feof(chunk_input)) {
        add_block(cur_size);
        fscanf(chunk_input, "%zu", &cur_size);
    }

    lastFit = root;
    fclose(chunk_input);
}

float getFragmentation() {
    size_t free = 0;
    size_t freeMax = 0;
    struct Segment *cur = root;

    while (cur) {
        if (!cur->used) {
            if (cur->size > freeMax) {
                freeMax = cur->size;
            }
            free += cur->size;
        }
        cur = cur->next;
    }

    size_t freeDiff = free - freeMax;

    if (freeDiff == free) {
        return 0;
    }

    return (float) freeDiff / free * 100;
}

struct timeval timerStart() {
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    return timeNow;
}

long timerStop(struct timeval timeStart) {
    struct timeval timeNow, timeResult;
    gettimeofday(&timeNow, NULL);
    timersub(&timeNow, &timeStart, &timeResult);
    return timeResult.tv_usec;
}

size_t total_requested(struct SizeNode *sizes) {
    size_t total = 0;
    while (sizes) {
        total += sizes->size;
        sizes = sizes->next;
    }
    return total;
}
size_t total_allocated(struct SizeNode *sizes) {
    size_t total = 0;
    while (sizes) {
        if (!sizes->failed) {
            total += sizes->size;
        }
        sizes = sizes->next;
    }
    return total;
}
void allocate_and_test_time(struct SizeNode *sizes, void *(*allocation_algorithm)(size_t)) {
    struct timeval startTime = timerStart();
    while (sizes) {
        if (allocation_algorithm(sizes->size) == NULL) {
            #if DEBUG
            printf("Allocation of size %zu failed!\n", sizes->size);
            #endif
            sizes->failed = 1;
        }
        #if DEBUG
        sizes->fragmentation = getFragmentation();
        printf("Fragmentation: %.1f%%\n", sizes->fragmentation);
        printMemory();
        #endif
        sizes = sizes->next;
    }

    long endTime = timerStop(startTime);
    printf("Time for allocation: %0ldus\n", endTime); /* Microseconds */
}

int main(int argc, char **argv) {
    char chunks_specified = 0;
    char sizes_specified = 0;
    char *chunk_file = NULL;
    char *size_file = NULL;

    int i = 1;
    for (; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            chunk_file = argv[++i];
            chunks_specified = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            size_file = argv[++i];
            sizes_specified = 1;
        }
    }

    if (!chunks_specified || !sizes_specified) {
        fprintf(stderr, "Argument -c or -s not specified\n");
        return -1;
    }

    read_chunks(chunk_file);
    struct SizeNode *sizes = read_request_sizes(size_file);

    #if DEBUG
    printChunks();
    print_sizes(sizes);
    printMemory();
    #endif

    printf("Initial fragmentation: %.1f%%\n", getFragmentation());

    allocate_and_test_time(sizes, NextFit);

    size_t requested = total_requested(sizes);
    size_t allocated = total_allocated(sizes);
    printf("Total requested/allocated: %zu/%zu\n", requested, allocated);

    printf("Fragmentation after test: %.1f%%\n", getFragmentation());

    return 0;
}