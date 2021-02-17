#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 30
#define MAXARGC 30

// Assuming that heap is 127 bytes long and memory is byte-addressable
size_t HEAP_SIZE = 127;

int find_free_block(unsigned char *Heap, size_t alloc_size) {
	// Finds a free block of memory using the "first fit" allocation strategy and returns the index of its header

	unsigned char *start = &Heap[0]; // Header address of the first block
	unsigned char *end = &Heap[HEAP_SIZE-1]; // Footer address of the last block
	end -= (*end >> 1) - 1; // Change pointer to address of last block's header
	unsigned char *p = start;

	// While not passed end and already allocated and too small
	while((p < end) && ((*p & 1) || (*p <= alloc_size)))
		p += (*p >> 1); // Go to the next block

	// If a block of requested size is not available.
	if ((*p & 1)) return -1;

	return p - start;
}


int add_block(unsigned char *Heap, size_t payload_size) {
	size_t alloc_size = payload_size + 2; // Additional space for storing header and footer

	if (payload_size <= 0 || alloc_size > HEAP_SIZE) return -1;

	int prev_index_header = find_free_block(Heap, alloc_size);
	if (prev_index_header == -1) return -1;

	size_t prev_alloc_size = Heap[prev_index_header] >> 1;

	// Add header and footer to the new block
	Heap[prev_index_header] = (alloc_size << 1) | 1; // Block size and allocation bit
	*(&Heap[prev_index_header] + alloc_size - 1) = (alloc_size << 1) | 1;

	if (alloc_size < prev_alloc_size) {
		unsigned char *header_remaining_block = &Heap[prev_index_header] + alloc_size;

		// Set length for the remaining part of the old block
		*header_remaining_block = (prev_alloc_size - alloc_size) << 1;
		*(header_remaining_block + (*header_remaining_block >> 1) - 1) = *header_remaining_block;
	}

	return prev_index_header+1;
}


int free_block(unsigned char *Heap, int index) {
	unsigned char  *cur_header, *cur_footer, *prev_header, *prev_footer, *next_header;
	unsigned char *p = &Heap[index-1]; // index-1 contains the header
	unsigned char prev_size = 0;

	// if not allocated
	if ((*p & 1) == 0) return -1;
	
	// Clear the allocated flags at header and footer to free the block
	*p &= -2;
	*(p + (*p >> 1) - 1) &= -2;

	// Backward Coalescing
	cur_header = p; // will stay same as p if backward coalescing does not happen
	prev_footer = p-1;
	while (prev_footer > Heap && (*prev_footer & 1) == 0) {
		prev_size = *cur_header >> 1; // Size of the block before header is moved left
		prev_header = prev_footer - (*prev_footer >> 1) + 1; // Gets address of header in the previous block
		cur_header = prev_header; // Header points to prev_header
		*cur_header = (prev_size + (*cur_header >> 1)) << 1; // Update the size of current header
		prev_footer -= (*prev_footer >> 1);
	}

	// Forward Coalescing
	next_header = cur_header + (*cur_header >> 1);
	while(next_header < &Heap[HEAP_SIZE-1] && (*next_header & 1) == 0) {
		*cur_header = ((*cur_header >> 1) + (*next_header >> 1)) << 1;
		next_header = cur_header + (*cur_header >> 1);
	}

	cur_footer = cur_header + (*cur_header >> 1) - 1; // Changes address to current header + size stored in current header
	*cur_footer = *cur_header; // Update size of current footer

	return 0;
}


void parseline(char* cmdline, char *argv[]) {
	const char delimiter[2] = " ";
	char *token;
	int counter = 0;

	// Remove trailing newline character from input
	char *temp_pos;
	if ((temp_pos = strchr(cmdline, '\n')) != NULL) *temp_pos = '\0';

	// strtok returns one token at a time
	token = strtok(cmdline, delimiter);
	while (token != NULL) {
		argv[counter++] = token;
		token = strtok(NULL, delimiter);
	}
}


void blocklist(unsigned char * Heap) {
	// print out blocks in order: pointer to start, payload size, allocation status

	unsigned char *start = &Heap[0]; // Header address of the first block
	unsigned char *p = start;
	unsigned char *end = &Heap[HEAP_SIZE-1]; // Footer address of the last block
	// while start < end, then print out the blocks
	while (p < end) {
		// print each block information:
		// pointer to start, payload size, allocation status
		// start,           *start & -2,   *start & 1

		if (*p & 1) { // if allocated
			printf("%ld, %d, allocated.\n", (p - start) + 1, (*p >> 1) - 2);
		}
		else { // if free
			printf("%ld, %d, free.\n", (p - start) + 1, (*p >> 1) - 2);
		}
		
		p += (*p >> 1); // goes to next block
	}

}


void writemem(unsigned char * Heap, int index, char * str) {
	unsigned char *target = &Heap[index];
	int size_of_str = strlen(str);
	int i = 0;
	for (i = 0; i < size_of_str; i++) {
	*target = str[i]; // char is written into address
	target += (*target >> 1);
	}
	
}


void printmem(unsigned char * Heap, int index, int num_chars_to_print) {
	unsigned char *target = &Heap[index];
	int i;
	for (i = 0; i < num_chars_to_print; i++) {
		if (i == num_chars_to_print - 1) {
			printf("%x\n", *target);
			break;
		}
		printf("%x ", *target); 
		target += (*target >> 1);
	}
}


int main() {
	char cmdline[MAXLINE];
	char *argv[MAXARGC];
	int index;

	// The heap is organized as an implicit free list. The heap is initially completely unallocated,
	// so it contains a single free block which is as big as the entire heap. Memory is initialized
	// so that all addresses (other than the header and the footer of the initial free block) contain 0.
	unsigned char *Heap = (unsigned char *) calloc(HEAP_SIZE, 1);

	// The most-significant 7 bits of the header and footer contain size of the block while the LSB indicates allocation bit.
	Heap[0] = HEAP_SIZE << 1; 
	Heap[HEAP_SIZE-1] = HEAP_SIZE << 1;

	while (1) {
		// Display prompt and get user input
		printf(">");
		fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin)) exit(0);

		parseline(cmdline, argv);
		
		if (strcmp(argv[0], "malloc") == 0) {
			if ((index = add_block(Heap, atoi(argv[1]))) == -1) 
				printf("Payload must be between %i and %i (inclusive).\n", 1, (int)HEAP_SIZE-2);
			else
				printf("%i\n", index);
		}
		else if (strcmp(argv[0], "free") == 0) {
			if (free_block(Heap, atoi(argv[1])) == -1)
				printf("Specified block cannot be freed (not allocated).\n");
		}
		else if (strcmp(argv[0], "blocklist") == 0) blocklist(Heap);
		else if (strcmp(argv[0], "writemem") == 0) writemem(Heap, atoi(argv[1]), argv[2]);
		else if (strcmp(argv[0], "printmem") == 0) printmem(Heap, atoi(argv[1]), atoi(argv[2]));
		else if (strcmp(argv[0], "quit") == 0) break;
		else printf("Invalid Input!\n");
	}

	return 0;
}