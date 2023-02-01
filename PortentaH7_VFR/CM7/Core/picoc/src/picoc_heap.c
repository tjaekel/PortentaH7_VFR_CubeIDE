/*
 * picoc_heap.c
 *
 * Created: 11/27/2011 5:42:26 PM
 *  Author: tjaekel
 */ 

/* stack grows up from the bottom and heap grows down from the top of heap space */
#include "picoc.h"

#define FREELIST_BUCKETS 8                          /* free lists for 4, 8, 12 ... 32 byte allocs */
#define SPLIT_MEM_THRESHOLD 16                      /* don't split memory which is close in size */

unsigned char bottom_of_heap[HEAP_SIZE] __attribute__((section(".sdram")));
#define  HeapMemory  bottom_of_heap
void *HeapStart = &HeapMemory[0];
/* static */ void *HeapBottom = &HeapMemory[HEAP_SIZE];   /* the bottom of the (downward-growing) heap */
static void *StackFrame = &HeapMemory[0];           /* the current stack frame */
void *HeapStackTop = &HeapMemory[0];                /* the top of the stack */

static struct AllocNode *FreeListBucket[FREELIST_BUCKETS] = {0};      /* we keep a pool of free list buckets to reduce fragmentation */
static struct AllocNode *FreeListBig;               /* free memory which doesn't fit in a bucket */

#ifdef DEBUG_HEAP
void ShowBigList(void)
{
    struct AllocNode *LPos;

    printf("Heap: bottom=0x%lx 0x%lx-0x%lx, big freelist=", (long)HeapBottom, (long)&HeapMemory[0], (long)&HeapMemory[HEAP_SIZE]);
    for (LPos = FreeListBig; LPos != NULL; LPos = LPos->NextFree)
        printf("0x%lx:%d ", (long)LPos, LPos->Size);

    printf("\n");
}
#endif

/* initialize the stack and heap storage */
void HeapInit(void)
{
    int Count;
    int AlignOffset = 0;

    FreeListBucket[0] = 0;
    FreeListBig = NULL;

    //initialize heap with 0, if startup code has not yet done
    memset(HeapMemory, 0, sizeof(HeapMemory));

    while (((PTR_TYPE_INT)&HeapMemory[AlignOffset] & (sizeof(ALIGN_TYPE)-1)) != 0)
        AlignOffset++;

    StackFrame = &HeapMemory[AlignOffset];
    HeapStackTop = &HeapMemory[AlignOffset];
    *(void **)StackFrame = NULL;
    HeapBottom = &HeapMemory[HEAP_SIZE-sizeof(ALIGN_TYPE)+AlignOffset];
    FreeListBig = NULL;
    for (Count = 0; Count < FREELIST_BUCKETS; Count++)
        FreeListBucket[Count] = NULL;
}

/* allocate some space on the stack, in the current stack frame
 * clears memory. Can return NULL if out of stack space */
void *HeapAllocStack(int Size)
{
    char *NewMem;
    char *NewTop;
    unsigned long x;

    NewMem = HeapStackTop;
    x = MEM_ALIGN(Size);
    NewTop = (char *)HeapStackTop + x;
#ifdef DEBUG_HEAP
    printf("HeapAllocStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size), (unsigned long)HeapStackTop);
#endif
    if (NewTop > (char *)HeapBottom)
    {
        //malloc error
        PlatformPrintf("FATAL: out of memory\n");
        PlatformExit();
        return NULL;
    }

    HeapStackTop = (void *)NewTop;
    memset((void *)NewMem, '\0', Size);
    return NewMem;
}

/* allocate some space on the stack, in the current stack frame */
void HeapUnpopStack(int Size)
{
#ifdef DEBUG_HEAP
    printf("HeapUnpopStack(%ld) at 0x%lx\n", (unsigned long)MEM_ALIGN(Size), (unsigned long)HeapStackTop);
#endif
    HeapStackTop = (void *)((char *)HeapStackTop + MEM_ALIGN(Size));
}

/* free some space at the top of the stack */
int HeapPopStack(void *Addr, int Size)
{
    int ToLose;

    ToLose = MEM_ALIGN(Size);
    if (ToLose > ((char *)HeapStackTop - (char *)&HeapMemory[0]))
        return FALSE;

#ifdef DEBUG_HEAP
    printf("HeapPopStack(0x%lx, %ld) back to 0x%lx\n", (unsigned long)Addr, (unsigned long)MEM_ALIGN(Size), (unsigned long)HeapStackTop - ToLose);
#endif
    HeapStackTop = (void *)((char *)HeapStackTop - ToLose);
    assert(Addr == NULL || HeapStackTop == Addr);

    return TRUE;
}

/* push a new stack frame on to the stack */
void HeapPushStackFrame(void)
{
#ifdef DEBUG_HEAP
    printf("Adding stack frame at 0x%lx\n", (unsigned long)HeapStackTop);
#endif
    *(void **)HeapStackTop = StackFrame;
    StackFrame = HeapStackTop;
    HeapStackTop = (void *)((char *)HeapStackTop + MEM_ALIGN(sizeof(ALIGN_TYPE)));
}

/* pop the current stack frame, freeing all memory in the frame. can return NULL */
int HeapPopStackFrame(void)
{
    if (*(void **)StackFrame != NULL)
    {
        HeapStackTop = StackFrame;
        StackFrame = *(void **)StackFrame;
#ifdef DEBUG_HEAP
        printf("Popping stack frame back to 0x%lx\n", (unsigned long)HeapStackTop);
#endif
        return TRUE;
    }
    else
        return FALSE;
}

/* allocate some dynamically allocated memory. memory is cleared. can return NULL if out of memory */
void *HeapAllocMem(int Size)
{
#ifdef USE_MALLOC_HEAP
    return calloc(Size, 1);
#else
    struct AllocNode *NewMem;
    struct AllocNode **FreeNode;
    unsigned int AllocSize;
    int Bucket;
    void *ReturnMem;

    NewMem = NULL;
    AllocSize = MEM_ALIGN(Size) + MEM_ALIGN(sizeof(NewMem->Size));
    if (Size == 0)
        return NULL;

    assert(Size > 0);

    /* make sure we have enough space for an AllocNode */
    if (AllocSize < sizeof(struct AllocNode))
        AllocSize = sizeof(struct AllocNode);

    Bucket = AllocSize >> 2;
    if (Bucket < FREELIST_BUCKETS && FreeListBucket[Bucket] != NULL)
    {
        /* try to allocate from a freelist bucket first */
#ifdef DEBUG_HEAP
        printf("allocating %d(%d) from bucket", Size, AllocSize);
#endif
        NewMem = FreeListBucket[Bucket];
        assert((PTR_TYPE_INT)NewMem >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)NewMem - &HeapMemory[0] < HEAP_SIZE);
        FreeListBucket[Bucket] = *(struct AllocNode **)NewMem;
        assert(FreeListBucket[Bucket] == NULL || ((PTR_TYPE_INT)FreeListBucket[Bucket] >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)FreeListBucket[Bucket] - &HeapMemory[0] < HEAP_SIZE));
        NewMem->Size = AllocSize;
    }
    else if (FreeListBig != NULL)
    {
        /* grab the first item from the "big" freelist we can fit in */
        for (FreeNode = &FreeListBig; *FreeNode != NULL && (*FreeNode)->Size < (unsigned int)AllocSize; FreeNode = &(*FreeNode)->NextFree)
        {}

        if (*FreeNode != NULL)
        {
            assert((PTR_TYPE_INT)*FreeNode >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)*FreeNode - &HeapMemory[0] < HEAP_SIZE);
            assert((*FreeNode)->Size < HEAP_SIZE && (*FreeNode)->Size > 0);
            if ((*FreeNode)->Size < (unsigned int)AllocSize + SPLIT_MEM_THRESHOLD)
            {
                /* close in size - reduce fragmentation by not splitting */
#ifdef DEBUG_HEAP
                printf("allocating %d(%d) from freelist, no split (%d)", Size, AllocSize, (*FreeNode)->Size);
#endif
                NewMem = *FreeNode;
                assert((PTR_TYPE_INT)NewMem >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)NewMem - &HeapMemory[0] < HEAP_SIZE);
                *FreeNode = NewMem->NextFree;
            }
            else
            {
                /* split this big memory chunk */
#ifdef DEBUG_HEAP
                printf("allocating %d(%d) from freelist, split chunk (%d)", Size, AllocSize, (*FreeNode)->Size);
#endif
                NewMem = (void *)((char *)*FreeNode + (*FreeNode)->Size - AllocSize);
                assert((PTR_TYPE_INT)NewMem >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)NewMem - &HeapMemory[0] < HEAP_SIZE);
                (*FreeNode)->Size -= AllocSize;
                NewMem->Size = AllocSize;
            }
        }
    }

    if (NewMem == NULL)
    {
        /* couldn't allocate from a freelist - try to increase the size of the heap area */
#ifdef DEBUG_HEAP
        printf("allocating %d(%d) at bottom of heap (0x%lx-0x%lx)", Size, AllocSize, (long)((char *)HeapBottom - AllocSize), (long)HeapBottom);
#endif
        if ((char *)HeapBottom - AllocSize < (char *)HeapStackTop)
        {
            PlatformPrintf("FATAL: out of memory\n");
            PlatformExit();
            return NULL;
        }

        HeapBottom = (void *)((char *)HeapBottom - AllocSize);
        NewMem = HeapBottom;
        NewMem->Size = AllocSize;
    }

    ReturnMem = (void *)((char *)NewMem + MEM_ALIGN(sizeof(NewMem->Size)));
    memset(ReturnMem, '\0', AllocSize - MEM_ALIGN(sizeof(NewMem->Size)));
#ifdef DEBUG_HEAP
    printf(" = %lx\n", (unsigned long)ReturnMem);
#endif
    return ReturnMem;
#endif
}

/* free some dynamically allocated memory */
void HeapFreeMem(void *Mem)
{
#ifdef USE_MALLOC_HEAP
    return free(Mem);
#else
    struct AllocNode *MemNode = (struct AllocNode *)((char *)Mem - MEM_ALIGN(sizeof(MemNode->Size)));
    int Bucket = MemNode->Size >> 2;

#ifdef DEBUG_HEAP
    printf("HeapFreeMem(0x%lx)\n", (unsigned long)Mem);
#endif
    assert((PTR_TYPE_INT)Mem >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)Mem - &HeapMemory[0] < HEAP_SIZE);
    assert(MemNode->Size < HEAP_SIZE && MemNode->Size > 0);
    if (Mem == NULL)
        return;

    if ((void *)MemNode == HeapBottom)
    {
        /* pop it off the bottom of the heap, reducing the heap size */
#ifdef DEBUG_HEAP
        printf("freeing %d from bottom of heap\n", MemNode->Size);
#endif
        HeapBottom = (void *)((char *)HeapBottom + MemNode->Size);
#ifdef DEBUG_HEAP
        ShowBigList();
#endif
    }
    else if (Bucket < FREELIST_BUCKETS)
    {
        /* we can fit it in a bucket */
#ifdef DEBUG_HEAP
        printf("freeing %d to bucket\n", MemNode->Size);
#endif
        assert(FreeListBucket[Bucket] == NULL || ((PTR_TYPE_INT)FreeListBucket[Bucket] >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)FreeListBucket[Bucket] - &HeapMemory[0] < HEAP_SIZE));
        *(struct AllocNode **)MemNode = FreeListBucket[Bucket];
        FreeListBucket[Bucket] = (struct AllocNode *)MemNode;
    }
    else
    {
        /* put it in the big memory freelist */
#ifdef DEBUG_HEAP
        printf("freeing %lx:%d to freelist\n", (unsigned long)Mem, MemNode->Size);
#endif
        assert(FreeListBig == NULL || ((PTR_TYPE_INT)FreeListBig >= (PTR_TYPE_INT)&HeapMemory[0] && (unsigned char *)FreeListBig - &HeapMemory[0] < HEAP_SIZE));
        MemNode->NextFree = FreeListBig;
        FreeListBig = MemNode;
#ifdef DEBUG_HEAP
        ShowBigList();
#endif
    }
#endif
}
