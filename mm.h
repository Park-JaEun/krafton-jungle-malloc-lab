#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

static void *extend_heap(size_t words);  // 힙 확장 함수 선언
static void *find_best_fit(size_t asize);     // 적합한 블록 찾기 함수 선언
static void place(void *bp, size_t asize);  // 블록 배치 함수 선언
static void *coalesce(void *bp);         // 블록 병합 함수 선언

/* 기본 상수 및 매크로 */
#define WSIZE 4                     // Word 크기
#define DSIZE 8                     // Double word 크기
#define CHUNKSIZE (1<<12)          // 힙 확장 크기
#define ALIGNMENT 8                 // 정렬 요구사항
#define CLASSES 20                  // 크기 클래스 개수

/* 매크로 함수들 */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* 가용 리스트 포인터 매크로 */
#define PRED_PTR(bp) ((char **)(bp))
#define SUCC_PTR(bp) ((char **)(bp + WSIZE))
#define PRED(bp) (*PRED_PTR(bp))
#define SUCC(bp) (*SUCC_PTR(bp))



/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

