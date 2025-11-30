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

#include "mm.h"
#include "memlib.h"

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


// For block header

#define OVERHEAD sizeof(block_header) + sizeof(block_footer)

/* Get to header from payload pointer bp */
#define HDRP(bp) ((char *) (bp) - sizeof(block_header))

/* Get to footer from payload pointer bp */
#define FTRP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)) - OVERHEAD)

#define GET_SIZE(p) ((block_header *) (p))->size

#define GET_ALLOC(p) ((block_header *) (p))->allocated

#define NEXT_BLKP(bp) ((char *) (bp) + GET_SIZE(HDRP(bp)))

#define PREV_BLKP(bp) ((char *) (bp) - GET_SIZE((char *)  (bp) - OVERHEAD))





void *current_avail = NULL;
size_t current_avail_size = 0;

/* Useful structs*/

/* Block header */
typedef struct {
  size_t size;
  char allocated;
} block_header;

/* Block footer with filler for allignment to 16 b */
typedef struct {
  size_t size;
  int filler;
} block_footer;

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


  sbrk(sizeof(block_header));
  first_bp = sbrk(0);

  GET_SIZE(HDRP(first_bp)) = 0;
  GET_ALLOC(HDRP(first_bp)) = 1;

  mm_malloc(0); // Never gets freed to avoid prev pointer error

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{

  // NAIVE
  // int newsize = ALIGN(size);
  // void *p;
  
  // if (current_avail_size < newsize) {
  //   current_avail_size = PAGE_ALIGN(newsize);
  //   current_avail = mem_map(current_avail_size);
  //   if (current_avail == NULL)
  //     return NULL;
  // }

  // p = current_avail;
  // current_avail += newsize;
  // current_avail_size -= newsize;
  
  // return p;

  int new_size = ALIGN(size + OVERHEAD);
  void *bp = f_bp;

  while (GET_SIZE(HDRP(bp)) != 0) {
    if (!GET_ALLOC(HDRP(bp))
      && (GET_SIZE(HDRP(bp)) >= new_size)) {
      set_allocated(bp, new_size);
      return bp;
    }
    bp = NEXT_BLKP(bp);
  }

  extend(new_size);
  set_allocated(bp, new_size);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(bp)) = 0;
  // coalesce(bp);
}

/* Helper Functions */

void extend(size_t new_size) {
  size_t chunk_size = CHUNK_ALIGN(new_size);
  void *bp = sbrk(chunk_size);

  GET_SIZE(HDRP(bp)) = chunk_size;
  GET_SIZE(FTRP(bp)) = chunk_size;
  GET_ALLOC(HDRP(bp)) = 0;

  GET_SIZE(HDRP(NEXT_BLKP(bp))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 1;

}

void set_allocated(void *bp, size_t size) {
  size_t extra_size = GET_SIZE(HDRP(bp)) - size;

  if (extra_size > ALIGN(1 + OVERHEAD)) {
    GET_SIZE(HDRP(bp)) = size;
    GET_SIZE(FTRP(bp)) = size;  
    GET_SIZE(HDRP(NEXT_BLKP(bp))) = extra_size;
    GET_SIZE(FTRP(NEXT_BLKP(bp))) = extra_size;
    GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0;
  }

  GET_ALLOC(HDRP(bp)) = 1;
}


void *coalesce (void *bp) {
  size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));


  // Case 1: Nothing to do

  if (prev_alloc && next_alloc) {
    /* Do nothing */
  }

  // Case 2: Freed block before another free block 

  else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    GET_SIZE(HDRP(bp)) = size;
    GET_SIZE(FTRP(bp)) = size;
  }

  // Case 3: Freed block after another free block

  else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    GET_SIZE(FTRP(bp)) = size;
    GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
    bp = PREV_BLKP(bp);
  }

  // Case 4: Freed block between free blocks

  else {
    size += (GET_SIZE(HDRP(PREV_BLKP(bp)))
              + GET_SIZE(HDRP(NEXT_BLKP(bp))));
    
    GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
    GET_SIZE(FTRP(NEXT_BLKP(bp))) = size;
    bp = PREV_BLKP(bp);
  }


  // Pointer to payload of coalesced free block
  return bp;
  
}
