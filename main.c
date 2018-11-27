#include <stdio.h>
#include <memory.h>

#define MY_BUFFER_SIZE 4096

unsigned char mybuffer[MY_BUFFER_SIZE];

void set_end(int num) {
    memcpy(&mybuffer[0], &num, sizeof(int));
}
int get_end() {
    int last;
    memcpy(&last, &mybuffer[0], sizeof(int));
    return last;
}
void *myalloc(size_t size) {
    // TODO - izmantot atbrīvoto vietu buferī
    size_t remaining_block_size = MY_BUFFER_SIZE - get_end();
    if (remaining_block_size < size + sizeof(int)) {
        fprintf(stderr, "Not enough space\n");
        return NULL;
    } else {
        printf("Allocating %zu bytes of memory.\n", size);
        // Saglabā bloka garumu
        memcpy(&mybuffer[get_end()], &size, sizeof(int));

        // Norāde uz alocētās atmiņas bloka sākumpunktu
        void *ptr = &mybuffer[get_end() + sizeof(int)];
        set_end(get_end() + size + sizeof(int));
        return ptr;
    }
};

int myfree(void *ptr) {
    int size_to_free;
    memcpy(&size_to_free, ptr - sizeof(int), sizeof(int));
    if (size_to_free == 0) {
        fprintf(stderr, "Nothing to clear\n");
        return -1;
    }
    printf("Deleting %i bytes of memory.\n", size_to_free);
    //Atbrīvo alocēto
    void * allocated = ptr;
    for (int i = 0; i < size_to_free; i++) {
        memset(allocated, 0, 1);
        allocated++;
    }
    //Atbrīvo atmiņas izmēra informāciju
    void * size_info = ptr - 1;
    for (int j = 0; j < sizeof(int); j++) {
        memset(size_info, 0, 1);
        size_info--;
    }

    return 0;
};

int main() {
    // Bufera sākumā glabā informāciju par tā izmantoto apjomu
    set_end(4);

    void *ptr = myalloc(2);
    void *ptr2 = myalloc(4);
    void *ptr3 = myalloc(6);

    //Trūkst vietas - atgriež NULL
    void *ptr4 = myalloc(4096);

    myfree(ptr);
    myfree(ptr2);
    myfree(ptr3);
    // Jau dzēsta atmiņa - atgriež -1
    myfree(ptr);

    printf("%s\n", mybuffer);

    return 0;
}