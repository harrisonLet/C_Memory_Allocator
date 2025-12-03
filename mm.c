/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * Building allocator, starting with naive approach.
 * Iteration 2: mmap based allocator with explicit free list.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* Debug flag: set to 0 to disable debug output, 1 to enable */
#define DEBUG 0

#if DEBUG
  #define DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(fmt, ...) (void)0
#endif

/* always use 16-byte alignment for blocks */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/* Get to header from payload pointer bp */
#define HDRP(bp) ((char *) (bp) - sizeof(block_header))

/* Get to footer from payload pointer bp */
  #define FTRP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)) - BLOCK_OVERHEAD)

  #define NEXT_BLKP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)))

  #define PREV_BLKP(bp) ((char *) (bp) - GET_SIZE((char *) (bp) - sizeof(block_footer)))

/* For packed headers/footers */
  #define GET(p) (*(size_t *) (p))

  #define GET_ALLOC(p) (GET(p) & 0x1)
  #define GET_SIZE(p) (GET(p) & ~0xF)

  #define PUT(p, val) (*(size_t *) (p) = (val))

  #define PACK(size, alloc) ((size) | (alloc))



/* MMap macros */

  #define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


  /* Page header structure access */
  #define GET_PAGE_SIZE(page) ((page_header *)(page))->page_size
  #define GET_NEXT_PAGE(page) ((page_header *)(page))->next



  /* Block overhead */
  #define BLOCK_OVERHEAD (sizeof(block_header) + sizeof(block_footer))

  /* Minimum block size (must fit free list pointers) */
  #define MIN_BLOCK_SIZE (2 * sizeof(void *) + BLOCK_OVERHEAD)

  /* Page overhead (page header + prologue + epilogue) */
  #define PAGE_OVERHEAD (BLOCK_OVERHEAD + BLOCK_OVERHEAD)


/* Explicit free list macros - access pointers stored in payload */
#define GET_NEXT_FREE(bp) (*(void **)(bp))
#define GET_PREV_FREE(bp) (*(void **)((char *)(bp) + sizeof(void *)))

#define SET_NEXT_FREE(bp, next) (GET_NEXT_FREE(bp) = (next))
#define SET_PREV_FREE(bp, prev) (GET_PREV_FREE(bp) = (prev))




/* Global variables */
void *current_avail = NULL;

void *first_bp = NULL;

void *first_page = NULL;

void *free_list_head = NULL;



/* Useful structs*/

/* Block header */
typedef size_t block_header;

/* Block footer  */
typedef size_t block_footer;


/* Forward declarations */
void *extend(size_t req_size);
void set_allocated(void *bp, size_t size);
void *coalesce(void *bp);
void add_to_free_list(void *bp);
void remove_from_free_list(void *bp);



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

  DEBUG_PRINT("[DEBUG] mm_init() called\n");
  
  first_page = NULL;

  free_list_head = NULL;
  
  extend(1);
 
  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  DEBUG_PRINT("[DEBUG] mm_malloc() - requested size=%zu\n", size);

  if (size == 0) {
      DEBUG_PRINT("[DEBUG] mm_malloc() - size is 0, returning NULL\n");
      return NULL;
  }

  int new_size = ALIGN(size + BLOCK_OVERHEAD);

  if (new_size < MIN_BLOCK_SIZE) {
      new_size = MIN_BLOCK_SIZE;
  }
  
  DEBUG_PRINT("[DEBUG] mm_malloc() - aligned size=%d, MIN_BLOCK_SIZE=%zu\n", new_size, MIN_BLOCK_SIZE);
    
  // Traverse through the free list

  void *bp = free_list_head;

  while (bp != NULL) {
    size_t block_size = GET_SIZE(HDRP(bp));
    DEBUG_PRINT("[DEBUG] mm_malloc() - checking free block bp=%p, size=%zu\n", bp, block_size);
    if (block_size >= new_size) {
        // Found a suitable block
        DEBUG_PRINT("[DEBUG] mm_malloc() - found suitable block, allocating\n");
        remove_from_free_list(bp);
        set_allocated(bp, new_size);
        DEBUG_PRINT("[DEBUG] mm_malloc() - returning allocated block bp=%p\n", bp);
        return bp;
    }
    bp = GET_NEXT_FREE(bp);
  }
  
  DEBUG_PRINT("[DEBUG] mm_malloc() - no suitable free block found, extending\n");
  // No suitable block found in any page, need to extend
  bp = extend(new_size);
  if (bp == NULL) {
      DEBUG_PRINT("[DEBUG] mm_malloc() - extend failed\n");
      return NULL;  // extend failed
  }

  DEBUG_PRINT("[DEBUG] mm_malloc() - extend successful, bp=%p\n", bp);
  remove_from_free_list(bp);
  set_allocated(bp, new_size);
  DEBUG_PRINT("[DEBUG] mm_malloc() - returning allocated block bp=%p\n", bp);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  if (ptr == NULL) {
      DEBUG_PRINT("[DEBUG] mm_free() - ptr is NULL, ignoring\n");
      return;
  }
  
  size_t block_size = GET_SIZE(HDRP(ptr));
  DEBUG_PRINT("[DEBUG] mm_free() - freeing block ptr=%p, size=%zu\n", ptr, block_size);
  
  PUT(HDRP(ptr), PACK(block_size, 0));  // Mark header as free
  PUT(FTRP(ptr), PACK(block_size, 0));  // Mark footer as free



  // ptr = coalesce(ptr);
  // add_to_free_list(ptr);
  // DEBUG_PRINT("[DEBUG] mm_free() - added to free list\n");
}

/* Helper Functions */

void *extend(size_t req_size) {
  DEBUG_PRINT("[DEBUG] extend() - requested size=%zu\n", req_size);

  //New page size required is the
  //passed in size + room for the page 
  //overhead which is terminator + page header + prologue

  // Size of new page, aligned to 4096 bytes
  size_t new_size = PAGE_ALIGN(req_size + PAGE_OVERHEAD);
  DEBUG_PRINT("[DEBUG] extend() - new_size after alignment=%zu (PAGE_OVERHEAD=%zu)\n", new_size, PAGE_OVERHEAD);

  // Request memory from mmap
  void *new_page = mem_map(new_size);
  if (new_page == NULL) {
    DEBUG_PRINT("[DEBUG] extend() - mem_map failed\n");
    return NULL;  // mmap failed
  }
  DEBUG_PRINT("[DEBUG] extend() - mem_map succeeded, new_page=%p\n", new_page);


  if (first_page == NULL) { // First page being added
      first_page = new_page;
  }


  // Move past the padding to start setting up blocks
  char *start = (char *)new_page;

  PUT(start, 0);

  char* prologue = start + BLOCK_OVERHEAD;

  // Set up prologue
  PUT(HDRP(prologue), PACK(BLOCK_OVERHEAD, 1));
  PUT(FTRP(prologue), PACK(BLOCK_OVERHEAD, 1));


  // Set up free block

  char* free_block = prologue + BLOCK_OVERHEAD;
  
  // Calculate size of the main free block
  size_t block_size = ALIGN(new_size - PAGE_OVERHEAD);

  PUT(HDRP(free_block), PACK(block_size, 0));
  PUT(FTRP(free_block), PACK(block_size, 0));

  
  DEBUG_PRINT("[DEBUG] extend() - main free block bp=%p, size=%zu\n", free_block, block_size);

  
  // Set up epilogue block (allocated, zero size) - marks end of page
  PUT(HDRP(NEXT_BLKP(free_block)), PACK(0, 1));  // Epilogue header
  DEBUG_PRINT("[DEBUG] extend() - epilogue set at offset %zu\n", (size_t)NEXT_BLKP(free_block) - (size_t)new_page);

  add_to_free_list(free_block);  // Add the new free block to the free list
  DEBUG_PRINT("[DEBUG] extend() - returning bp=%p\n", free_block);
  
  return free_block;  // Return pointer to payload of the free block
}



void set_allocated(void *bp, size_t size) {
  size_t block_size = GET_SIZE(HDRP(bp));  // Get ORIGINAL block size first
  size_t remaining_size = block_size - size;  // Leftover space in block after allocation
  
  DEBUG_PRINT("[DEBUG] set_allocated() - bp=%p, requested size=%zu, actual block_size=%zu, remaining=%zu\n", 
          bp, size, block_size, remaining_size);
  
  if (remaining_size >= MIN_BLOCK_SIZE) {
    DEBUG_PRINT("[DEBUG] set_allocated() - splitting block, allocated=%zu, free=%zu\n", size, remaining_size);
    PUT(HDRP(bp), PACK(size, 1));  // Set allocated block header
    PUT(FTRP(bp), PACK(size, 1));  // Set allocated block footer

    void *remain = NEXT_BLKP(bp);  // Move to the remaining free block
    PUT(HDRP(remain), PACK(remaining_size, 0));  // Set free for remainder
    PUT(FTRP(remain), PACK(remaining_size, 0));  // Set free footer for remainder

    add_to_free_list(remain);  // Add remaining free block to free list
    DEBUG_PRINT("[DEBUG] set_allocated() - remainder added to free list at=%p\n", remain);
  } 
  
  else {
    DEBUG_PRINT("[DEBUG] set_allocated() - no split, using entire block of size=%zu\n", block_size);
    // No remaining block space
    PUT(HDRP(bp), PACK(block_size, 1));  // Allocate entire block
    PUT(FTRP(bp), PACK(block_size, 1));  // Allocate entire block footer  
  }
}


void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  
  // Don't coalesce if next block is epilogue (size 0)
  if (GET_SIZE(HDRP(NEXT_BLKP(bp))) == 0) {
    next_alloc = 1;  // Treat epilogue as allocated (can't coalesce)
  }

  // Case 1: Nothing to do
  if (prev_alloc && next_alloc) {
    return bp;
  }

  // Case 2: Coalesce with next block (only if not epilogue)
  else if (prev_alloc && !next_alloc && GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0) {
    void *next_bp = NEXT_BLKP(bp);
    size += GET_SIZE(HDRP(next_bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // Case 3: Coalesce with previous block
  else if (!prev_alloc && next_alloc) {
    void *prev_bp = PREV_BLKP(bp);
    size += GET_SIZE(HDRP(prev_bp));
    PUT(HDRP(prev_bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    bp = prev_bp;
  }

  // Case 4: Coalesce with both (only if next is not epilogue)
  else if (!prev_alloc && !next_alloc && GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0) {
    void *prev_bp = PREV_BLKP(bp);
    void *next_bp = NEXT_BLKP(bp);
    size += GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(next_bp));
    PUT(HDRP(prev_bp), PACK(size, 0));
    PUT(FTRP(next_bp), PACK(size, 0));
    bp = prev_bp;
  }

  return bp;
}



void add_to_free_list(void *bp) {
    DEBUG_PRINT("[DEBUG] add_to_free_list() - adding bp=%p, size=%zu, old head=%p\n", 
            bp, GET_SIZE(HDRP(bp)), free_list_head);
    
    SET_NEXT_FREE(bp, free_list_head);
    SET_PREV_FREE(bp, NULL);
    
    if (free_list_head != NULL) {
        SET_PREV_FREE(free_list_head, bp);
    }
    
    free_list_head = bp;
    DEBUG_PRINT("[DEBUG] add_to_free_list() - new head=%p\n", free_list_head);
}

void remove_from_free_list(void *bp) {
    void *next = GET_NEXT_FREE(bp);
    void *prev = GET_PREV_FREE(bp);
    
    DEBUG_PRINT("[DEBUG] remove_from_free_list() - removing bp=%p, prev=%p, next=%p\n", bp, prev, next);
    
    // Update previous block's next pointer (or head if bp is first)
    if (prev == NULL) {
        free_list_head = next;  // bp was the head
        DEBUG_PRINT("[DEBUG] remove_from_free_list() - was head, new head=%p\n", free_list_head);
    } else {
        SET_NEXT_FREE(prev, next);
    }
    
    // Update next block's prev pointer
    if (next != NULL) {
        SET_PREV_FREE(next, prev);
    }
}
