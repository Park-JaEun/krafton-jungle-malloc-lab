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
 * 
 * 
 * segregated 리스트 기반 메모리 할당자
 * 각 크기 클래스별로 독립적인 연결 리스트를 유지하고 검색 시간을 최적화 함.
 * 
 * 1. segregated 가용 리스트로 빠른 검색
 * 2. 즉시 병합으로 외부 단편화 최소화
 * 3. Best-Fit 검색으로 내부 단편화 감소
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    "Team 12",
    "Jaeun Park",
    "ddoavoca@gmail.com",
    "",
    ""
};


/* 전역 변수 */
static char *heap_listp = NULL;     // 힙의 시작 포인터
static char *segregated_lists[CLASSES]; // 크기별 가용 리스트 배열

/* 함수 선언 */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_node(void *bp, size_t size);
static void remove_node(void *bp);
static int get_class(size_t size);

/**
 * mm_init - 메모리 할당자 초기화
 */
int mm_init(void) {
    // 초기 빈 힙 생성
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    // 초기화 블록들 설정
    PUT(heap_listp, 0);                            // 정렬 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  // 프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  // 프롤로그 풋터
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));      // 에필로그 헤더
    heap_listp += (2*WSIZE);

    // 세그리게이티드 리스트 초기화
    for (int i = 0; i < CLASSES; i++)
        segregated_lists[i] = NULL;

    // 초기 가용 블록으로 힙 확장
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    
    return 0;
}

/**
 * find_fit - 요청 크기에 맞는 가용 블록 검색(Best-Fit -> 같은 클래스 내에서는 First-Fit)
 * @param asize: 할당할 크기
 * @return: 적합한 블록의 포인터 또는 NULL
 */
static void *find_fit(size_t asize) {
    int class = get_class(asize);
    
    // 적절한 크기 클래스부터 검색 시작
    while (class < CLASSES) {
        void *bp = segregated_lists[class];
        
        // 현재 클래스 내에서 검색
        while (bp != NULL) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
            bp = SUCC(bp);
        }
        class++;
    }
    return NULL;
}

/**
 * place - 가용 블록에 요청 크기를 할당
 * @param bp: 할당할 블록의 포인터
 * @param asize: 할당할 크기
 */
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    remove_node(bp);

    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_node(bp, csize-asize);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/**
 * mm_malloc - 메모리 할당
 * @param size: 할당할 크기
 * @return: 할당된 블록의 포인터
 */
void *mm_malloc(size_t size) {
    size_t asize;      // 조정된 블록 크기
    size_t extendsize; // 힙 확장 크기
    char *bp;

    if (size == 0)
        return NULL;

    // 최소 블록 크기 보장 및 정렬 요구사항 충족
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    // 적합한 블록 검색
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // 적합한 블록이 없으면 힙을 확장
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    
    place(bp, asize);
    return bp;
}

/**
 * mm_free - 블록 해제
 * @param bp: 해제할 블록의 포인터
 */
void mm_free(void *bp) {
    if (bp == NULL) return;

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_node(bp, size);
    coalesce(bp);
}

/**
 * coalesce - 인접한 가용 블록들을 병합
 * @param bp: 병합할 블록의 포인터
 * @return: 병합된 블록의 포인터
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        remove_node(bp);
        remove_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {
        remove_node(bp);
        remove_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        remove_node(bp);
        remove_node(PREV_BLKP(bp));
        remove_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    insert_node(bp, size);
    return bp;
}

/**
 * mm_realloc - 블록 크기 재조정
 * @param ptr: 재조정할 블록의 포인터
 * @param size: 새로운 크기
 * @return: 재조정된 블록의 포인터
 */
void *mm_realloc(void *ptr, size_t size) {
    void *newptr;
    size_t copySize;
    
    if ((newptr = mm_malloc(size)) == NULL)
        return NULL;

    if (ptr != NULL) {
        copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
        if (size < copySize)
            copySize = size;
        memcpy(newptr, ptr, copySize);
        mm_free(ptr);
    }

    return newptr;
}



/**
 * extend_heap - 힙을 확장하고 새로운 가용 블록을 생성
 * @param words: 확장할 워드 수
 * @return: 새로 생성된 블록의 포인터 또는 에러 시 NULL
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // 더블 워드 정렬을 위해 짝수 워드로 조정
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // 새 가용 블록의 헤더와 풋터 초기화
    PUT(HDRP(bp), PACK(size, 0));          // 가용 블록 헤더
    PUT(FTRP(bp), PACK(size, 0));          // 가용 블록 풋터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 새 에필로그 헤더

    // 새로운 가용 블록을 적절한 가용 리스트에 추가
    insert_node(bp, size);

    // 이전 블록과 병합이 가능한 경우 병합 수행
    return coalesce(bp);
}


/**
 * get_class - 주어진 크기에 해당하는 크기 클래스 인덱스를 반환
 * @param size: 블록 크기
 * @return: 크기 클래스 인덱스
 */
static int get_class(size_t size) {
    int class = 0;
    size = size - 1; // size가 2의 거듭제곱일 때의 경계 처리
    while (size > 1) {
        size >>= 1;
        class++;
    }
    return class < CLASSES ? class : CLASSES - 1;
}

/**
 * insert_node - 가용 블록을 적절한 크기 클래스의 리스트에 삽입
 * @param bp: 삽입할 블록의 포인터
 * @param size: 블록의 크기
 */
static void insert_node(void *bp, size_t size) {
    int class = get_class(size);
    void *search_ptr = segregated_lists[class];
    void *insert_ptr = NULL;

    // 크기 순서로 정렬된 위치 찾기
    while (search_ptr != NULL && size > GET_SIZE(HDRP(search_ptr))) {
        insert_ptr = search_ptr;
        search_ptr = SUCC(search_ptr);
    }

    // 리스트에 노드 삽입
    if (insert_ptr != NULL) {
        SUCC(bp) = search_ptr;
        PRED(bp) = insert_ptr;
        if (search_ptr != NULL)
            PRED(search_ptr) = bp;
        SUCC(insert_ptr) = bp;
    } else {
        SUCC(bp) = segregated_lists[class];
        PRED(bp) = NULL;
        if (segregated_lists[class] != NULL)
            PRED(segregated_lists[class]) = bp;
        segregated_lists[class] = bp;
    }
}

/**
 * remove_node - 가용 리스트에서 노드 제거
 * @param bp: 제거할 블록의 포인터
 */
static void remove_node(void *bp) {
    int class = get_class(GET_SIZE(HDRP(bp)));
    
    if (PRED(bp) != NULL)
        SUCC(PRED(bp)) = SUCC(bp);
    else
        segregated_lists[class] = SUCC(bp);
    
    if (SUCC(bp) != NULL)
        PRED(SUCC(bp)) = PRED(bp);
}