#include <stdio.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MY_BUFFER_SIZE 1024

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

void *NextFit(size_t size) {
    if (size == 0) {
        return NULL;
    }

    struct Segment *cur = lastFit;

    do {
        if (!cur->used && size <= cur->size) {

            if (size < cur->size) {
                // Place new empty segment between current and next segment
                struct Segment *split = malloc(sizeof(struct Segment));// Shhhh

                split->prev = cur;
                split->next = cur->next;
                split->size = cur->size - size;
                split->used = 0;
                split->address = cur->address + size;

                if (cur->next) {
                    cur->next->prev = split;
                }
                cur->next = split;
            }

            cur->used = 1;
            cur->size = size;

            lastFit = cur;

            return cur->address;
        }

        if (cur->next) {
            // Go forward
            cur = cur->next;
        } else {
            // Loop around
            cur = root;
        }
    } while (cur != lastFit);

    return NULL;
}

void printMemory() {
    struct Segment *cur = root;

    while (cur) {
        printf("|");
        for (int i = 0; i < (int) cur->size - 1; i++) {
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

struct SizeNode { // TODO: Merge this with segment
    struct SizeNode *next;
    size_t size;
} sizeNode;

struct SizeNode *create_size_node(size_t size) {
    struct SizeNode *tmp = calloc(1, sizeof sizeNode);
    tmp->size = size;
    tmp->next = NULL;
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
    fclose(chunk_input);

    lastFit = root;
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

    read_chunks(chunk_file);
    printChunks();

    struct SizeNode *sizes = read_request_sizes(size_file);
    print_sizes(sizes);
    printMemory();

    // TODO: Pass needed allocation algorithm instead. Functions needs same interface
    void *(*allocation_algorithm)(size_t) = NextFit;

    while (sizes) {
        if (allocation_algorithm(sizes->size) == NULL) {
            printf("Allocation of size %zu failed!\n", sizes->size);
        } else {
            printf("Allocating size: %zu\n", sizes->size);
        }
        printMemory();
        sizes = sizes->next;
    }

    return 0;
}