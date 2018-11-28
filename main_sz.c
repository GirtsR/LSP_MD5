#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MY_BUFFER_SIZE 1024
#define BLOCK_SIZE 16
#define BLOCK_COUNT MY_BUFFER_SIZE / BLOCK_SIZE 

unsigned char mybuffer[MY_BUFFER_SIZE];

struct Segment* root;
struct Segment* lastFit;
size_t lastFitCount;

struct Segment {
	struct Segment* prev;
	struct Segment* next;
	size_t blockCount;// Multiply by BLOCK_SIZE to get size in bytes
	char used;
};

void* myalloc(size_t size) {
	if (size == 0) {
		return NULL;
	}
	 
	struct Segment* cur = lastFit;
	size_t curCount = lastFitCount;// Avoid recount
	size_t neededCount = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

	do {
		if (!cur->used && neededCount <= cur->blockCount) {

			if (neededCount < cur->blockCount) {
				// Place new empty segment between current and next segment
				struct Segment* split = malloc(sizeof(struct Segment));// Shhhh

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
		}
		else {
			// Loop around
			curCount = 0;
			cur = root;
		}
	} while (cur != lastFit);

	return NULL;
}

int myfree(void* ptr) {

	struct Segment* cur = root;
	void* freeOffset = (void*)(ptr - (void*)mybuffer);
	void* curOffset = 0;

	while (cur) {
		if (freeOffset == curOffset) {
			if (!cur->used) {
				return -1;
			}

			// Free segment
			cur->used = 0;

			// Merge with previous free segment (delete prev)
			if (cur->prev && !cur->prev->used) {
				struct Segment* prevPrev = cur->prev->prev;

				if (prevPrev) {
					prevPrev->next = cur;
				}

				if (cur->prev == lastFit) {
					lastFit = cur;
				}

				if (cur->prev == root) {
					root = cur;
				}

				cur->blockCount += cur->prev->blockCount;

				free(cur->prev);
				cur->prev = prevPrev;
			}

			// Merge with next free segment (delete next)
			if (cur->next && !cur->next->used) {
				struct Segment* nextNext = cur->next->next;

				if (nextNext) {
					nextNext->prev = cur;
				}

				if (cur->next == lastFit) {
					lastFit = cur;
				}

				cur->blockCount += cur->next->blockCount;

				free(cur->next);
				cur->next = nextNext;
			}

			return 0;
		}

		// Keep looking
		curOffset += cur->blockCount * BLOCK_SIZE;
		cur = cur->next;
	}

	return -1;
}

void printMemory() {
	struct Segment* cur = root;

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

int main() {
	setup();

	void* a = myalloc(128);
	void* b = myalloc(256);
	void* c = myalloc(512);
	
	printMemory();

	myfree(a);
	myfree(c);
		
	printMemory();

	void* d = myalloc(64);
	
	printMemory();

	myfree(b);
	
	printMemory();

	void* e = myalloc(128);
	
	printMemory();

	/*
	// Memory randomizer
	while (1) {
		int free = rand() % 4;
		int size = rand() % MY_BUFFER_SIZE;
		void* ptr = mybuffer + (rand() % BLOCK_COUNT) * BLOCK_SIZE;

		if (!free) {
			if (myalloc(size)) {
				printf("ALLOC ");
				printMemory();
				usleep(100000);
			}
		}
		else {
			if (myfree(ptr) != -1) {
				printf("FREE  ");
				printMemory();
				usleep(100000);
			}
		}
	}*/

	return 0;
}