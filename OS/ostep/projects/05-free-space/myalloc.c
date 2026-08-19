/*
 * myalloc.c — 프리 리스트 기반 메모리 할당기 직접 구현 (OSTEP ch17)
 *
 * 구현 요소
 *   1. mmap 으로 고정 크기 힙 확보 (원서 17.2 "Embedding A Free List")
 *   2. 할당 블록마다 헤더(size + magic) 부착 → free 가 크기를 알아내는 방법
 *   3. 프리 리스트를 힙 안에 임베딩 (리스트 노드용 별도 malloc 불가)
 *   4. 분할(splitting) · 병합(coalescing)
 *   5. fit 정책 3종 — best / worst / first
 *
 * 목적: malloc 라이브러리가 내부에서 무엇을 하는지 직접 만들어 확인
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#define HEAP_SIZE 4096
#define MAGIC     1234567

/* 할당된 블록 앞에 붙는 헤더 (원서 17.2) */
typedef struct {
    int size;    // 사용자에게 준 바이트 수 (헤더 제외)
    int magic;   // 무결성 검사용
} header_t;

/* 프리 리스트 노드 — 빈 공간 안에 직접 놓인다 */
typedef struct __node_t {
    int              size;   // 이 빈 청크에서 쓸 수 있는 바이트 수 (노드 자체 제외)
    struct __node_t *next;
} node_t;

typedef enum { FIT_BEST, FIT_WORST, FIT_FIRST } policy_t;

static node_t  *head;
static void    *heap_start;
static policy_t policy = FIT_BEST;

/* ---------- 초기화 ---------- */

static void heap_init(void) {
    /*
     * mmap 인자
     *   NULL                    커널이 주소 선택
     *   HEAP_SIZE               길이
     *   PROT_READ|PROT_WRITE    읽기·쓰기 허용
     *   MAP_ANON|MAP_PRIVATE    파일과 무관한 익명 매핑, 사설(복사-쓰기)
     *   -1, 0                   익명 매핑이므로 fd 와 오프셋 미사용
     */
    heap_start = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);
    assert(heap_start != MAP_FAILED);

    head       = (node_t *) heap_start;
    head->size = HEAP_SIZE - sizeof(node_t);   // 4096 - 16 (arm64 정렬 결과)
    head->next = NULL;
}

/* ---------- 프리 리스트 출력 ---------- */

static void dump_free_list(const char *label) {
    printf("%-28s ", label);
    node_t *cur = head;
    if (!cur) { printf("(빈 리스트)\n"); return; }
    int total = 0, count = 0;
    while (cur) {
        printf("[off:%ld len:%d]", (long) ((char *) cur - (char *) heap_start), cur->size);
        total += cur->size;
        count++;
        cur = cur->next;
        if (cur) printf(" -> ");
    }
    printf("   (청크 %d개, 빈 공간 %d바이트)\n", count, total);
}

/* ---------- fit 정책에 따른 청크 선택 ---------- */

static node_t *find_chunk(int need, node_t **prev_out) {
    node_t *cur = head, *prev = NULL;
    node_t *pick = NULL, *pick_prev = NULL;

    while (cur) {
        if (cur->size >= need) {
            if (policy == FIT_FIRST) {                       // 첫 적합 즉시 반환
                pick = cur; pick_prev = prev; break;
            }
            if (!pick) { pick = cur; pick_prev = prev; }
            else if (policy == FIT_BEST  && cur->size < pick->size) { pick = cur; pick_prev = prev; }
            else if (policy == FIT_WORST && cur->size > pick->size) { pick = cur; pick_prev = prev; }
        }
        prev = cur;
        cur  = cur->next;
    }
    *prev_out = pick_prev;
    return pick;
}

/* ---------- 할당 ---------- */

void *my_malloc(int size) {
    if (size <= 0) return NULL;

    /* 사용자 요청 N 바이트에 대해 실제로는 N + 헤더 크기를 찾아야 한다 (원서 17.2) */
    int need = size + (int) sizeof(header_t);

    node_t *prev;
    node_t *chunk = find_chunk(need, &prev);
    if (!chunk) return NULL;                            // 충족 불가 → NULL

    int remain = chunk->size - need;

    if (remain >= (int) sizeof(node_t)) {
        /* 분할(splitting) — 남은 부분이 노드를 담을 만큼 크면 프리 리스트에 유지 */
        node_t *rest = (node_t *) ((char *) chunk + need);
        rest->size = remain;
        rest->next = chunk->next;
        if (prev) prev->next = rest; else head = rest;
    } else {
        /* 남는 자리가 너무 작으면 청크 전체를 소비 (내부 단편화 발생) */
        if (prev) prev->next = chunk->next; else head = chunk->next;
    }

    header_t *h = (header_t *) chunk;
    h->size  = size;
    h->magic = MAGIC;
    return (void *) (h + 1);                            // 헤더 다음이 사용자 영역
}

/* ---------- 해제 + 병합 ---------- */

void my_free(void *ptr) {
    if (!ptr) return;

    /* 포인터 산술로 헤더 위치 역산 (원서 17.2) */
    header_t *h = (header_t *) ptr - 1;
    assert(h->magic == MAGIC);                          // 무결성 검사

    node_t *blk = (node_t *) h;
    int     len = h->size + (int) sizeof(header_t);

    /* 주소 순서를 유지하며 삽입 — 병합을 쉽게 하려는 목적 (원서 17.3 first-fit 논의) */
    node_t *cur = head, *prev = NULL;
    while (cur && cur < blk) { prev = cur; cur = cur->next; }

    blk->size = len;
    blk->next = cur;
    if (prev) prev->next = blk; else head = blk;

    /* 병합(coalescing) — 뒤쪽 이웃과 인접하면 합친다 */
    if (blk->next && (char *) blk + blk->size == (char *) blk->next) {
        blk->size += blk->next->size;
        blk->next  = blk->next->next;
    }
    /* 앞쪽 이웃과 인접하면 합친다 */
    if (prev && (char *) prev + prev->size == (char *) blk) {
        prev->size += blk->size;
        prev->next  = blk->next;
    }
}

/* ---------- 데모 ---------- */

static const char *policy_name(void) {
    switch (policy) {
        case FIT_BEST:  return "best fit";
        case FIT_WORST: return "worst fit";
        default:        return "first fit";
    }
}

static void demo_split_coalesce(void) {
    printf("\n===== 1. 분할과 병합 =====\n");
    heap_init();
    dump_free_list("초기 상태");

    void *a = my_malloc(100);
    dump_free_list("my_malloc(100) 후");
    void *b = my_malloc(100);
    dump_free_list("my_malloc(100) 후");
    void *c = my_malloc(100);
    dump_free_list("my_malloc(100) 후");

    my_free(b);
    dump_free_list("가운데 블록 해제 후");
    my_free(a);
    dump_free_list("첫 블록 해제 후 (병합)");
    my_free(c);
    dump_free_list("마지막 해제 후 (전부 병합)");
}

static void demo_policy(policy_t p) {
    printf("\n===== 2. fit 정책: %s =====\n", policy_name());
    heap_init();
    policy = p;

    /*
     * 원서 17.3 예제(프리 청크 10 / 30 / 20)를 재현.
     * 큰 블록 사이에 작은 블록을 끼워 두고 작은 것만 해제하면
     * 서로 떨어진 크기가 다른 프리 청크들이 남는다.
     * 순서를 30 / 10 / 20 으로 두어 세 정책이 각기 다른 청크를 고르게 만든다.
     */
    void *p1 = my_malloc(200);   void *g1 = my_malloc(30);
    void *p2 = my_malloc(200);   void *g2 = my_malloc(10);
    void *p3 = my_malloc(200);   void *g3 = my_malloc(20);
    my_free(g1); my_free(g2); my_free(g3);
    (void) p1; (void) p2; (void) p3;
    dump_free_list("요청 전 프리 리스트");

    void *x = my_malloc(8);      // 헤더 8 + 8 = 16 → 세 청크 모두 수용 가능
    printf("my_malloc(8) -> off:%ld\n", (long) ((char *) x - (char *) heap_start));
    dump_free_list("요청 후 프리 리스트");
}

int main(int argc, char *argv[]) {
    printf("sizeof(header_t) = %zu, sizeof(node_t) = %zu\n",
           sizeof(header_t), sizeof(node_t));

    demo_split_coalesce();

    policy = FIT_BEST;  demo_policy(FIT_BEST);
    policy = FIT_WORST; demo_policy(FIT_WORST);
    policy = FIT_FIRST; demo_policy(FIT_FIRST);

    printf("\n===== 3. 할당 실패 =====\n");
    heap_init();
    policy = FIT_BEST;
    void *huge = my_malloc(HEAP_SIZE);      // 헤더까지 고려하면 불가능한 크기
    printf("my_malloc(%d) -> %s\n", HEAP_SIZE, huge ? "성공" : "NULL (실패)");
    return 0;
}
