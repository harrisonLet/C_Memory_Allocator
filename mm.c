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
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment for blocks */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


/* Additional constants and macros*/

#define CHUNK_SIZE (1<<14) // Chunks are 4 pages

/* rounds up to the nearest multiple of CHUNK_SIZE */
#define CHUNK_ALIGN(size) (((size) + (CHUNK_SIZE-1)) & ~(CHUNK_SIZE-1))


// For block header

#define OVERHEAD sizeof(block_header) + sizeof(block_footer)

/* Get to header from payload pointer bp */
#define HDRP(bp) ((char *) (bp) - sizeof(block_header))

/* Get to footer from payload pointer bp */
  #define FTRP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

  #define NEXT_BLKP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)))

  #define PREV_BLKP(bp) ((char *) (bp) - GET_SIZE((char *)  (bp) - OVERHEAD))

/* For packed headers/footers */
  #define GET(p) (*(size_t *) (p))

  #define GET_ALLOC(p) (GET(p) & 0x1)
  #define GET_SIZE(p) (GET(p) & ~0xF)

  #define PUT(p, val) (*(size_t *) (p) = (val))

  #define PACK(size, alloc) ((size) | (alloc))



/* MMap macros */

  #define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))


  /* Page header structure access */
  #define GET_PAGE_HEADER(bp) ((page_header *)((char *)(bp) - sizeof(page_header)))
  #define GET_PAGE_SIZE(page) ((page_header *)(page))->size
  #define GET_NEXT_PAGE(page) ((page_header *)(page))->next



  /* Block overhead */
  #define BLOCK_OVERHEAD (sizeof(block_header) + sizeof(block_footer))

  /* Minimum block size (must fit free list pointers) */
  #define MIN_BLOCK_SIZE (2 * sizeof(void *) + BLOCK_OVERHEAD)

  /* Page overhead (page header + prologue + epilogue) */
  #define PAGE_OVERHEAD (sizeof(page_header) + 2 * BLOCK_OVERHEAD)



/* Global variables */
void *current_avail = NULL;
size_t current_avail_size = 0;

void *first_bp = NULL;

void *first_page = NULL;
void *last_page = NULL;


/* Useful structs*/

/* Block header */
typedef size_t block_header;

/* Block footer  */
typedef size_t block_footer;

/* Page header */
typedef struct page_header {
    struct page_header *next;  // Pointer to next page
    size_t page_size;          // Total size of this page
} page_header;


/* Forward declarations */
void *extend(size_t req_size);
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

  first_page = NULL;
  extend(1);

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{

 int new_size = ALIGN(size + BLOCK_OVERHEAD);
    
    // Traverse through each page
    page_header *current_page = (page_header *)first_page;    

    while (current_page != NULL) {
        // Start at the first block in this page
        char *page_start = (char *)current_page + sizeof(page_header);
        char *bp = page_start + BLOCK_OVERHEAD;  // Skip prologue
        
        // Traverse blocks within this page until we hit the epilogue (size == 0)
        while (GET_SIZE(HDRP(bp)) != 0) {
            if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) {
                // Found a suitable free block
                set_allocated(bp, new_size);
                return bp;
            }
            bp = NEXT_BLKP(bp);  // Move to next block in this page
        }
        
        // Didn't find space in this page, move to next page
        current_page = current_page->next;
    }
    
    // No suitable block found in any page, need to extend
    void *bp = extend(new_size);
    if (bp == NULL) {
        return NULL;  // extend failed
    }
    set_allocated(bp, new_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));  // Mark header as free
  PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));  // Mark footer as free
  // coalesce(ptr);
}

/* Helper Functions */

void *extend(size_t req_size) {

  //New page size required is the
  //passed in size + room for the page 
  //overhead which is terminator + page header + prologue

  // Size of new page, aligned to 4096 bytes
  size_t new_size = PAGE_ALIGN(req_size + PAGE_OVERHEAD);

  // Request memory from mmap
  void *new_page = mem_map(new_size);
  if (new_page == NULL) {
    return NULL;  // mmap failed
  }

  // Set up the page header at the beginning
  page_header *page_hdr = (page_header *)new_page;
  page_hdr->page_size = new_size;
  page_hdr->next = NULL;


  if (first_page == NULL) { // First page being added
      first_page = page_hdr;
      last_page = page_hdr;
  } else {
      page_header *last = (page_header *)last_page; // Retrieve last page header
      last->next = page_hdr;  // Link to end of list
      last_page = page_hdr;  // Update last_page to new page
  }


  // Move past the page header to start setting up blocks
  char *start = (char *)new_page + sizeof(page_header);
  
  // Set up prologue block (allocated, minimal size)
  PUT(start, PACK(BLOCK_OVERHEAD, 1));  // Prologue header
  PUT(start + sizeof(block_header), PACK(BLOCK_OVERHEAD, 1));  // Prologue footer
  
  // Move to the main free block
  char *bp = start + BLOCK_OVERHEAD;  // bp points to payload of first real block
  
  // Calculate size of the main free block
  size_t block_size = new_size - sizeof(page_header) - 2 * BLOCK_OVERHEAD;
  
  // Set up the main free block (unallocated)
  PUT(HDRP(bp), PACK(block_size, 0));  // Block header
  PUT(FTRP(bp), PACK(block_size, 0));  // Block footer
  
  // Set up epilogue block (allocated, zero size) - marks end of page
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // Epilogue header
  
  return bp;  // Return pointer to payload of the free block
}



void set_allocated(void *bp, size_t size) {
  size_t block_size = GET_SIZE(HDRP(bp));  // Get ORIGINAL block size first
  
  // Update header and footer for the allocated portion
  PUT(HDRP(bp), PACK(size, 1));
  PUT(FTRP(bp), PACK(size, 1));
  
  // If there's leftover space, split and mark it as free
  if (block_size > size) {
    size_t remaining_size = block_size - size;
    void *next_bp = NEXT_BLKP(bp);
    PUT(HDRP(next_bp), PACK(remaining_size, 0));  // Free block
    PUT(FTRP(next_bp), PACK(remaining_size, 0));
  }
}


// void *coalesce (void *bp) {
//   size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
//   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
//   size_t size = GET_SIZE(HDRP(bp));


//   // Case 1: Nothing to do

//   if (prev_alloc && next_alloc) {
//     /* Do nothing */
//   }

//   // Case 2: Freed block before another free block 

//   else if (prev_alloc && !next_alloc) {
//     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
//     GET_SIZE(HDRP(bp)) = size;
//     GET_SIZE(FTRP(bp)) = size;
//   }

//   // Case 3: Freed block after another free block

//   else if (!prev_alloc && next_alloc) {
//     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
//     GET_SIZE(FTRP(bp)) = size;
//     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
//     bp = PREV_BLKP(bp);
//   }

//   // Case 4: Freed block between free blocks

//   else {
//     size += (GET_SIZE(HDRP(PREV_BLKP(bp)))
//               + GET_SIZE(HDRP(NEXT_BLKP(bp))));
    
//     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
//     GET_SIZE(FTRP(NEXT_BLKP(bp))) = size;
//     bp = PREV_BLKP(bp);
//   }


//   // Pointer to payload of coalesced free block
//   return bp;
  
// }
