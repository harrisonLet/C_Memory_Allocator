# Malloc Implementation Status

## ✅ Completed Features

1. **Switched from sbrk to mmap** - Using `mem_map()` and `mem_unmap()` as required
2. **16-byte alignment** - Payload pointers are properly aligned
3. **Chunk tracking system** - Linked list of chunks to traverse non-contiguous memory
4. **Block coalescing** - Merges adjacent free blocks (with chunk boundary checks)
5. **Block headers/footers** - Using implicit free list structure
6. **Best-fit allocation** - Searches for smallest suitable free block

## ⚠️ Current Issues

1. **Payload overlap bugs** - Same block being allocated twice (needs debugging)
2. **Segfaults** - Likely due to boundary issues or invalid memory access

## 📋 Recommended Improvements (Per Assignment Tips)

### High Priority for Performance:

1. **Explicit Free List** ⭐⭐⭐
   - Current: Implicit list (searches ALL blocks every time - O(n))
   - Recommended: Maintain explicit list of free blocks only
   - Impact: Much faster allocation (O(1) insert, faster search)
   - Implementation: Add prev/next pointers in free block payload space

2. **Page Unmapping** ⭐⭐⭐
   - Current: Never unmaps pages (hurts instantaneous space utilization)
   - Recommended: Unmap completely empty chunks when all blocks freed
   - Impact: Improves instantaneous space utilization score significantly
   - Implementation: Check if chunk is completely free, call `mem_unmap()`

3. **Fix Current Bugs** ⭐⭐⭐
   - Payload overlap errors suggest blocks not marked as allocated correctly
   - Segfaults need to be debugged and fixed

### Medium Priority:

4. **First-fit vs Best-fit**
   - Current: Best-fit (searches all blocks)
   - Consider: First-fit might be faster with explicit free list

5. **Chunk Size Strategy**
   - Current: Fixed CHUNK_SIZE (16KB)
   - Consider: Dynamic sizing based on request size

### Optional (For Debugging):

6. **Heap Consistency Checker**
   - Add function to validate heap structure
   - Check: block headers, free list integrity, no overlaps, etc.
   - Remove before submission (slows down performance)

## Performance Score Breakdown

Your score depends on:
- **Space Utilization (30%)**: Ratio of peak allocated memory to heap size
- **Instantaneous Space Utilization (30%)**: Geometric mean over all operations (needs unmapping!)
- **Throughput (40%)**: Operations per second (needs explicit free list!)

Target: P = 0.47 for 75%, P = 0.70 for 100%

## Next Steps

1. **Fix current bugs first** (overlap, segfault)
2. **Add explicit free list** for throughput
3. **Add page unmapping** for instantaneous utilization
4. **Test incrementally** with small traces first

