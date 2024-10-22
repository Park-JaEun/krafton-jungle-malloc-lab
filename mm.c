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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 힙의 시작 주소를 가리킬 포인터
static char *heap_listp;

// 함수 선언
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;  // 확장 실패 시 -1 반환

    // 패딩 워드 설정 (정렬을 위한 공간)
    PUT(heap_listp, 0); 
    
    // 프롤로그 블록 설정 (2 워드 크기). 프롤로그는 할당된 상태로 유지됩니다.
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));  // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));  // 프롤로그 풋터
    
    // 에필로그 블록 설정 (크기 0, 할당 상태)
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); 
    
    // heap_listp를 첫 번째 유효 블록(프롤로그 다음)으로 이동
    heap_listp += (2 * WSIZE);

    // 힙을 CHUNKSIZE(4096바이트)만큼 확장하여 가용 블록을 생성
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;  // 확장 실패 시 -1 반환
    return 0;  // 성공 시 0 반환
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; // 요청된 크기를 정렬된 크기로 조정한 값 (할당할 블록 크기)
    size_t extendsize; // 힙을 확장할 때 필요한 크기
    char *bp; // 블록 포인터 (가용 블록을 가리킴)
    
    if (size == 0)  // 요청된 크기가 0이면 NULL을 반환 (메모리 할당을 하지 않음)
     return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE; // 최소 블록 크기 설정 (헤더 + 풋터를 포함한 최소 블록은 16바이트)
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); // 8바이트 정렬된 크기로 조정

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize); // 적합한 가용 블록을 찾으면 그 위치에 블록을 배치
        return bp; // 배치 후 해당 블록의 포인터를 반환
    }

    extendsize = MAX(asize, CHUNKSIZE); // 확장 크기는 요청한 크기와 CHUNKSIZE 중 더 큰 값
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL; // 힙 확장에 실패한 경우 NULL 반환

    place(bp, asize); // 새로 확장한 블록에 요청된 크기만큼 배치
    return bp; // 확장된 블록의 포인터 반환
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr;
    size_t oldsize;

    // ptr이 NULL이면, 새로 할당 -> malloc처럼 동작
    if (ptr == NULL)
        return mm_malloc(size);

    // size가 0이면, 할당 해제 -> free처럼 동작
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    // 기존 블록의 크기를 헤더에서 가져옴
    oldsize = GET_SIZE(HDRP(ptr)) - DSIZE; // 헤더와 풋터 제외한 페이로드 크기

    // 요청한 크기가 기존 블록 크기보다 작거나 같으면, 그대로 사용
    if (size <= oldsize) {
        return ptr;
    }

    // 기존 블록 크기가 충분하지 않으면 새로 할당
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    // 기존 블록의 데이터를 새 블록으로 복사 (페이로드 크기만 복사)
    memcpy(newptr, ptr, oldsize);

    // 기존 블록 해제
    mm_free(ptr);

    return newptr;
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));  // 헤더에서 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  // 다음 블록 헤더에서 확인
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {           // 이전 + 다음 블록 할당
        return bp;
    } else if (prev_alloc && !next_alloc) {   // 다음 블록 가용 
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {   // 이전 블록 가용 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                  // 이전 + 다음 블록 모두 가용 
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // 요청한 크기를 8바이트로 정렬
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    // 힙을 확장하고 실패 시 NULL 반환
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    // 새로 확장한 블록의 헤더와 풋터를 설정
    PUT(HDRP(bp), PACK(size, 0));         // 가용 블록 헤더
    PUT(FTRP(bp), PACK(size, 0));         // 가용 블록 풋터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // 새로운 에필로그 블록 설정

    // 새로 확장한 블록을 병합한 후 반환
    return coalesce(bp);
}

static void *find_fit(size_t asize) {
    void *bp;

    // First-fit 방식으로 가용 블록을 탐색
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;  // 적합한 블록을 찾으면 반환
        }
    }
    return NULL;  // 적합한 블록이 없으면 NULL 반환
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));  // 현재 블록의 크기 가져오기

    // 블록이 충분히 크면 분할 (최소 블록 크기 이상)
    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));  // 헤더에 할당 표시
        PUT(FTRP(bp), PACK(asize, 1));  // 풋터에 할당 표시

        // 나머지 공간을 가용 블록으로 설정 (최소 크기 이상의 가용 블록)
        bp = NEXT_BLKP(bp);  // 다음 블록으로 이동
        PUT(HDRP(bp), PACK(csize - asize, 0));  // 남은 블록을 가용 블록으로 설정
        PUT(FTRP(bp), PACK(csize - asize, 0));  // 가용 블록의 풋터 설정
    } else {
        PUT(HDRP(bp), PACK(csize, 1));  // 전체 블록을 할당 상태로 설정
        PUT(FTRP(bp), PACK(csize, 1));  // 풋터 설정
    }
}