/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * Building allocator, starting with naive approach.
 * Iteration 1: chunk based allocator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"



/* Useful structs*/

/* Block header and footer are just integers storing packed size/alloc info */
/* Size of header/footer is the size of an int (or size_t) */
#define WSIZE sizeof(int)  // Word size (header and footer size)

/* Chunk descriptor - tracks each mmap'd chunk
 * Stored at the beginning of each chunk mapping
 */
typedef struct chunk_node {
  struct chunk_node *next;    // Next chunk in the list (NULL if last)
  size_t chunk_size;          // Size of the chunk
} chunk_node_t;

/* Global chunk list head */
static chunk_node_t *chunk_list = NULL;




/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


/* Additional constants and macros*/

#define CHUNK_SIZE (1<<14) // Chunks are 4 pages

/* rounds up to the nearest multiple of CHUNK_SIZE */
#define CHUNK_ALIGN(size) (((size) + (CHUNK_SIZE-1)) & ~(CHUNK_SIZE-1))


// For block header and footer

#define OVERHEAD (WSIZE + WSIZE)  // Header + Footer

/* Get to header from payload pointer bp */
#define HDRP(bp) ((char *) (bp) - WSIZE)

/* Get to footer from payload pointer bp */
#define FTRP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

#define GET_SIZE(p) (GET(p) & ~0xF)

#define GET_ALLOC(p) (GET(p) & 0x1)

#define NEXT_BLKP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)))

#define PREV_BLKP(bp) ((char *) (bp) - GET_SIZE((char *)  (bp) - OVERHEAD))

#define GET(p) (*(int *) (p))

/* For left size operations of header and footer information */

#define PUT(p, val) (*(int *)(p) = (val))

#define PACK(size, alloc) ((size) | (alloc))


void *current_avail = NULL;
size_t current_avail_size = 0;
void *first_bp = NULL;  // First block pointer for heap traversal


/* Offset from chunk base to first block (after chunk_node) */
#define CHUNK_NODE_SIZE ALIGN(sizeof(chunk_node_t))

/* Forward function declarations */
static void add_chunk(void *chunk_base, size_t chunk_size);
static void *get_first_block_in_chunk(chunk_node_t *chunk_node);
static int is_block_in_chunk(void *bp, chunk_node_t *chunk_node);
void *extend(size_t new_size);
void set_allocated(void *bp, size_t size);
void *coalesce(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  // Butterfly knife guy mminit
  // Freelist head null
  // something else null
  // extend by1 
  // return return -1 if failed 0 if success

  // Initialize chunk list
  chunk_list = NULL;
  
  // Allocate initial chunk for sentinel block
  size_t init_size = CHUNK_SIZE;
  void *chunk = mem_map(init_size);
  if (chunk == NULL) {
    return -1;
  }
  
  // Add chunk to the list (chunk_node is at start of chunk)
  add_chunk(chunk, init_size);
  
  // Set up prologue block (first block position, marks start)
  // This ensures we have a valid header to read when traversing
  chunk_node_t *chunk_node = (chunk_node_t *)chunk;
  void *prologue = get_first_block_in_chunk(chunk_node);
  PUT(HDRP(prologue), PACK(0, 1));  // Size 0, allocated (acts as sentinel)
  
  // Set up epilogue block at the end (marks end of chunk)
  void *epilogue = (char *)chunk + init_size - WSIZE;
  PUT(HDRP(epilogue), PACK(0, 1));  // Size 0, allocated
  
  first_bp = NULL;

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  int new_size = ALIGN(size + OVERHEAD);
  void *best_bp = NULL;
  chunk_node_t *chunk_node;
  void *bp;
  size_t best_size = 0;

  // Search through all chunks for best fit
  for (chunk_node = chunk_list; chunk_node != NULL; chunk_node = chunk_node->next) {
    // Start at the first block in this chunk
    bp = get_first_block_in_chunk(chunk_node);
    
    // Safety check: ensure bp is valid before accessing header
    if (!is_block_in_chunk(bp, chunk_node)) {
      continue;  // Invalid chunk, skip
    }
    
    // Skip prologue if it exists (size 0) - also handles empty chunks
    if (GET_SIZE(HDRP(bp)) == 0) {
      continue;  // Empty chunk or prologue, skip it
    }
    
    // Traverse blocks within this chunk until we hit the epilogue (size 0)
    while (GET_SIZE(HDRP(bp)) != 0) {
      // Check if block is free and large enough
      if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) {
        // Check if this is a better fit (smaller or first match)
        if (!best_bp || (GET_SIZE(HDRP(bp)) < best_size)) {
          best_bp = bp;
          best_size = GET_SIZE(HDRP(bp));
        }
      }
      
      // Move to next block in this chunk
      bp = NEXT_BLKP(bp);
      
      // Safety check: ensure we haven't left the chunk boundaries
      if (!is_block_in_chunk(bp, chunk_node)) {
        break;
      }
      
      // Additional safety: if size is 0, we've hit epilogue
      if (GET_SIZE(HDRP(bp)) == 0) {
        break;
      }
    }
  }

  // If we found a suitable block, use it
  if (best_bp) {
    set_allocated(best_bp, new_size);
    return best_bp;
  }

  // No suitable block found, extend heap with new chunk
  bp = extend(new_size);
  if (bp == NULL) {
    return NULL;
  }
  set_allocated(bp, new_size);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  void *bp = ptr;
  PUT(HDRP(bp), GET(HDRP(bp)) & ~0x1);  // Clear allocated bit
  // coalesce(bp);
}

/* Helper Functions */

/* Add a chunk to the chunk list - chunk_node is embedded at start of chunk */
static void add_chunk(void *chunk_base, size_t chunk_size) {
  chunk_node_t *chunk_node = (chunk_node_t *)chunk_base;
  chunk_node->next = chunk_list;
  chunk_node->chunk_size = chunk_size;
  chunk_list = chunk_node;
}

/* Get the first block pointer in a chunk (after chunk_node and header) */
/* Ensures payload pointer is 16-byte aligned */
static void *get_first_block_in_chunk(chunk_node_t *chunk_node) {
  void *chunk_base = (void *)chunk_node;
  // Start after chunk_node
  uintptr_t after_chunk = (uintptr_t)chunk_base + CHUNK_NODE_SIZE;
  
  // We want payload to be 16-byte aligned
  // Payload will be at: after_chunk + header_size + padding
  // So: align(after_chunk + WSIZE + padding) = 16-byte boundary
  // Simplest: align the position right after chunk_node + header
  uintptr_t tentative_payload = after_chunk + WSIZE;
  uintptr_t aligned_payload = ALIGN(tentative_payload);
  
  // If alignment moved us forward, we need padding
  // Header goes right before aligned payload
  return (void *)aligned_payload;
}

/* Get chunk_base from chunk_node */
static void *get_chunk_base(chunk_node_t *chunk_node) {
  return (void *)chunk_node;
}

/* Check if a block pointer is within a chunk */
static int is_block_in_chunk(void *bp, chunk_node_t *chunk_node) {
  void *chunk_base = get_chunk_base(chunk_node);
  // bp points to payload, which starts after chunk_node and header
  void *first_bp = get_first_block_in_chunk(chunk_node);
  void *chunk_end = (char *)chunk_base + chunk_node->chunk_size;
  // Check if bp is at or after first block and before end of chunk
  return ((char *)bp >= (char *)first_bp && (char *)bp < (char *)chunk_end);
}

void *extend(size_t new_size) {
  size_t chunk_size = CHUNK_ALIGN(new_size);
  void *chunk = mem_map(chunk_size);
  if (chunk == NULL) {
    return NULL;  // Handle error
  }
  
  // Add chunk to the list (chunk_node is at start of chunk)
  add_chunk(chunk, chunk_size);
  
  // Get chunk node and first block
  chunk_node_t *chunk_node = (chunk_node_t *)chunk;
  void *bp = get_first_block_in_chunk(chunk_node);
  
  // Set up epilogue block at the end of this chunk first
  void *epilogue = (char *)chunk + chunk_size - WSIZE;
  PUT(HDRP(epilogue), PACK(0, 1));
  
  // Calculate block size: total size from header start to epilogue header start
  // Block size stored includes: header + payload + footer
  // Header is at bp - WSIZE, epilogue header is at epilogue
  void *header_start = HDRP(bp);
  size_t block_size = (uintptr_t)epilogue - (uintptr_t)header_start;
  
  PUT(HDRP(bp), PACK(block_size, 0));
  PUT(FTRP(bp), PACK(block_size, 0));
  
  return bp;
}

void set_allocated(void *bp, size_t size) {
  // Get the current block size before modifying
  size_t current_size = GET_SIZE(HDRP(bp));
  size_t extra_size = current_size - size;

  if (extra_size > ALIGN(OVERHEAD)) {
    // Split block: allocate current block, create free block from remainder
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    
    void *next_bp = NEXT_BLKP(bp);
    PUT(HDRP(next_bp), PACK(extra_size, 0));
    PUT(FTRP(next_bp), PACK(extra_size, 0));
  } else {
    // No split, just mark as allocated - preserve the current size
    PUT(HDRP(bp), PACK(current_size, 1));
    PUT(FTRP(bp), PACK(current_size, 1));
  }
}


/* Helper to find which chunk a block belongs to */
static chunk_node_t *find_chunk_for_block(void *bp) {
  chunk_node_t *chunk_node;
  for (chunk_node = chunk_list; chunk_node != NULL; chunk_node = chunk_node->next) {
    if (is_block_in_chunk(bp, chunk_node)) {
      return chunk_node;
    }
  }
  return NULL;  // Block not found in any chunk
}

void *coalesce (void *bp) {
  chunk_node_t *chunk_node = find_chunk_for_block(bp);
  if (chunk_node == NULL) {
    return bp;  // Block not in any chunk, can't coalesce
  }
  
  void *chunk_base = get_chunk_base(chunk_node);
  void *first_bp = get_first_block_in_chunk(chunk_node);
  
  // Check if we can coalesce with previous block (must be in same chunk)
  int can_coalesce_prev = 0;
  size_t prev_alloc = 1;  // Default to allocated if no prev block
  if ((char *)bp > (char *)first_bp) {
    // There might be a previous block
    void *prev_bp = PREV_BLKP(bp);
    if (is_block_in_chunk(prev_bp, chunk_node)) {
      prev_alloc = GET_ALLOC(HDRP(prev_bp));
      can_coalesce_prev = 1;
    }
  }
  
  // Check next block (must not be epilogue)
  void *next_bp = NEXT_BLKP(bp);
  size_t next_alloc = 1;  // Default to allocated
  if (is_block_in_chunk(next_bp, chunk_node) && GET_SIZE(HDRP(next_bp)) != 0) {
    next_alloc = GET_ALLOC(HDRP(next_bp));
  }
  
  size_t size = GET_SIZE(HDRP(bp));


  // Case 1: Nothing to do

  if (prev_alloc && next_alloc) {
    /* Do nothing */
  }

  // Case 2: Freed block before another free block 

  else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(next_bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // Case 3: Freed block after another free block (only if can coalesce with prev)

  else if (can_coalesce_prev && !prev_alloc && next_alloc) {
    void *prev_bp = PREV_BLKP(bp);
    size += GET_SIZE(HDRP(prev_bp));
    bp = prev_bp;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // Case 4: Freed block between free blocks

  else if (can_coalesce_prev && !prev_alloc && !next_alloc) {
    void *prev_bp = PREV_BLKP(bp);
    size += (GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(next_bp)));
    bp = prev_bp;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }


  // Pointer to payload of coalesced free block
  return bp;
  
}
