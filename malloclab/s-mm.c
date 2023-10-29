/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "No.1",
    /* First member's full name */
    "Avery Zhou",
    /* First member's email address */
    "Avery@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define CHUNKSIZE (1 << 12)  /* Extend heap by this amount (bytes) */

#define WSIZE 4     /* Word and header/footer size (bytes) */
#define DSIZE 8     /* Double word size (bytes) */
#define ctg_num 20  /* The number of catagory */

/* set the size and alloc bit */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))   // void*不可以直接进行间接引用 所以需要强制类型转换
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define SET(p, bp) (*(unsigned int *)(p) = (unsigned int)(uintptr_t)(bp))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Find the head of each empty list catagory */
#define GET_HEAD(num) ((unsigned int *)(long)(GET(heap_listp + WSIZE * num)))

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
// char* 指针允许以字节为单位操作内存, 因而便于访问内存中的任何位置
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given bp, find the precursor and successor */
#define GET_PRE(bp) ((unsigned int *)(long)(GET(bp)))
#define GET_SUC(bp) ((unsigned int *)(long)(GET(bp + WSIZE)))

static char *heap_listp; // Point to the start of catagory block
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static int search(size_t size);    
static void insert(void *bp);  
static void delete(void *bp);  


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk((4 + ctg_num) * WSIZE)) == (void *)-1) return -1;

    for (int i = 0; i < ctg_num; i++) {
        SET(heap_listp + i * WSIZE, NULL);
    }

    PUT(heap_listp + (ctg_num * WSIZE), 0);                     /* Alignment padding */
    PUT(heap_listp + ((1 + ctg_num) * WSIZE), PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_listp + ((2 + ctg_num) * WSIZE), PACK(DSIZE, 1));  /* Prologue footer */
    PUT(heap_listp + ((3 + ctg_num) * WSIZE), PACK(0, 1));      /* Epilogue header */
    
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}


static void* extend_heap(size_t words) {
    char* bp; // pointer to the payload
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void*)-1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));          /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));          /* Free block footer */
    PUT(HDRP(NEXT_BP(bp)), PACK(0, 1));    /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/* Find the catagory matches the given size */
static int search(size_t size) {
    int num = 0;
    while (size > 1) {
        size = size >> 1;
        num++;
    }
    return num - 4;
}


static void insert(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);
    if (GET_HEAD(num) == NULL) {
        SET(heap_listp + num * WSIZE, bp); // 分类块内放的是对应空闲链表的头地址
        /* Precursor */
        SET(bp, NULL);      
        /* Successor */              
        SET(bp + WSIZE, NULL);    
    } else {
        SET(bp + WSIZE, GET_HEAD(num));
        SET(GET_HEAD(num), bp);
        SET(bp, NULL);
        SET(heap_listp + num * WSIZE, bp);
    }
}


static void delete(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int num = search(size);

    /* ONLY one node */
    if (!GET_PRE(bp) && !GET_SUC(bp)) {
        SET(heap_listp + num * WSIZE, NULL);

    /* Last node */
    } else if (GET_PRE(bp) && !GET_SUC(bp)) {
        SET(GET_PRE(bp) + 1, NULL);

    /* First node */
    } else if (!GET_PRE(bp) && GET_SUC(bp)) {
        SET(heap_listp + num * WSIZE, GET_SUC(bp));
        SET(GET_SUC(bp), NULL);

    /* Middle node */
    } else {
        SET(GET_PRE(bp) + 1, GET_SUC(bp));
        SET(GET_SUC(bp), GET_PRE(bp));
    }
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{   /*
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
    */
    
    size_t asize;            /* Adjusted block size */
    size_t extendsize;       /* Amount to extend heap if no fit */
    char* bp;

    /* Ignore spurious requests */
    if (size == 0) return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE) asize = 2 * DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}


static void *find_fit(size_t asize) {
    void *bp;
    int num = search(asize);
    while (num < ctg_num) {
        bp = GET_HEAD(num);
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize) {
                return bp;
            }
            bp = GET_SUC(bp);
        }
        num++;
    }
    return NULL;
}


static void place(void *bp, size_t asize) {
    size_t total_size = GET_SIZE(HDRP(bp));

    delete(bp);
    if ((total_size - asize) >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BP(bp);
        PUT(HDRP(bp), PACK(total_size - asize, 0));
        PUT(FTRP(bp), PACK(total_size - asize, 0));  
        insert(bp);
    }
    else {
        PUT(HDRP(bp), PACK(total_size, 1));
        PUT(FTRP(bp), PACK(total_size, 1));
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}


static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 */
    if (prev_alloc && next_alloc) {
        insert(bp);
        return bp;
    }
    /* Case 2 */
    else if (!prev_alloc && next_alloc) {
        delete(PREV_BP(bp));
        size += GET_SIZE(HDRP(PREV_BP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BP(bp)), PACK(size, 0));
        bp = PREV_BP(bp);
    }
    /* Case 3 */
    else if (prev_alloc && !next_alloc) {
        delete(NEXT_BP(bp));
        size += GET_SIZE(HDRP(NEXT_BP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    /* Case 4 */
    else {
        /*
        size += GET_SIZE(HDRP(PREV_BP(bp))) + GET_SIZE(HDRP(NEXT_BP(bp)));
        bp = PREV_BP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        */
        delete(NEXT_BP(bp));
        delete(PREV_BP(bp));
        size += GET_SIZE(HDRP(PREV_BP(bp))) + GET_SIZE(FTRP(NEXT_BP(bp)));
        PUT(HDRP(PREV_BP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BP(bp)), PACK(size, 0));
        bp = PREV_BP(bp);
    }

    insert(bp);
    return bp;
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    /*
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
    */

    void *newptr;
    size_t copySize;
    
    if((newptr = mm_malloc(size)) == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(ptr));
    if(size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}