/*
 * replace.c — 페이지 교체 정책 시뮬레이터 (OSTEP ch22)
 *
 * 구현 정책 5종
 *   OPT   : 최적(Belady MIN) — 가장 먼 미래에 접근될 페이지를 축출. 미래를 알아야 하므로 비현실적
 *   FIFO  : 먼저 들어온 페이지를 축출
 *   LRU   : 가장 오래 전에 접근된 페이지를 축출
 *   CLOCK : use 비트 + 시계 바늘로 LRU 근사
 *   RAND  : 무작위 축출
 *
 * 재현 대상 (원서 Figure 22.1~22.9)
 *   1. 원서 예제 트레이스 0,1,2,0,1,3,0,3,1,2,1 (캐시 3) 의 정책별 히트율
 *   2. 워크로드 3종 (지역성 없음 / 80-20 / 순환 순차) 의 캐시 크기별 히트율
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXCACHE 128

typedef enum { OPT, FIFO, LRU, CLOCK, RAND } policy_t;
static const char *pname[] = { "OPT", "FIFO", "LRU", "CLOCK", "RAND" };

/* 결정적 의사 난수 — Math.random 대신 직접 구현해 재현성을 확보 */
static unsigned long rng_state = 12345;
static unsigned long rng(void) {
    rng_state = rng_state * 6364136223846793005UL + 1442695040888963407UL;
    return rng_state >> 33;
}

/*
 * 트레이스를 주어진 캐시 크기·정책으로 시뮬레이션하고 히트 수를 반환.
 * verbose 가 참이면 단계별 표를 출력 (원서 Figure 22.1~22.5 재현용)
 */
static int simulate(const int *trace, int n, int csize, policy_t p, int verbose) {
    int cache[MAXCACHE], used[MAXCACHE];   /* used = CLOCK 의 use 비트 */
    long stamp[MAXCACHE];                  /* LRU 최근 접근 시각 / FIFO 진입 순서 */
    int  count = 0, hand = 0, hits = 0;
    long clock_tick = 0;

    if (verbose)
        printf("  %-7s %-9s %-6s %s\n", "Access", "Hit/Miss?", "Evict", "Cache State");

    for (int t = 0; t < n; t++) {
        int page = trace[t];
        int found = -1;
        for (int i = 0; i < count; i++) if (cache[i] == page) { found = i; break; }

        int evicted = -1;

        if (found >= 0) {
            hits++;
            /* LRU 만 히트 시 시각을 갱신한다.
             * FIFO 는 '진입 순서' 이므로 히트해도 갱신하지 않는다 —
             * 이 한 줄 차이가 FIFO 와 LRU 를 가른다 */
            if (p == LRU) stamp[found] = ++clock_tick;
            used[found]  = 1;               /* CLOCK use 비트 세트 */
        } else if (count < csize) {
            cache[count] = page;            /* 아직 자리가 있음 = 강제 미스 */
            stamp[count] = ++clock_tick;
            used[count]  = 1;
            count++;
        } else {
            int victim = 0;
            switch (p) {
            case OPT: {
                /* 각 캐시 페이지가 다음에 언제 다시 쓰이는지 찾아 가장 먼 것을 고른다 */
                int far = -1;
                for (int i = 0; i < count; i++) {
                    int next = n;                       /* 다시 안 쓰이면 무한대로 취급 */
                    for (int k = t + 1; k < n; k++) if (trace[k] == cache[i]) { next = k; break; }
                    if (next > far) { far = next; victim = i; }
                }
                break;
            }
            case FIFO:
            case LRU: {
                /* FIFO 는 '진입 시각', LRU 는 '최근 접근 시각' 이 가장 작은 것.
                 * 두 경우 모두 stamp 최소값이나, 갱신 시점이 다르다
                 * (FIFO 는 진입 때만, LRU 는 히트 때도 갱신) */
                long min = stamp[0]; victim = 0;
                for (int i = 1; i < count; i++) if (stamp[i] < min) { min = stamp[i]; victim = i; }
                break;
            }
            case CLOCK: {
                /* use 비트가 0 인 페이지를 찾을 때까지 바늘을 돌리며 1 을 0 으로 지운다 */
                while (used[hand % count]) {
                    used[hand % count] = 0;
                    hand = (hand + 1) % count;
                }
                victim = hand % count;
                hand = (hand + 1) % count;
                break;
            }
            case RAND:
                victim = (int) (rng() % (unsigned long) count);
                break;
            }
            evicted      = cache[victim];
            cache[victim] = page;
            stamp[victim] = ++clock_tick;
            used[victim]  = 1;
        }

        if (verbose) {
            printf("  %-7d %-9s ", page, (found >= 0) ? "Hit" : "Miss");
            if (evicted >= 0) printf("%-6d ", evicted); else printf("%-6s ", "");
            /* 캐시 내용을 페이지 번호 오름차순으로 정렬해 출력 (비교 편의) */
            int sorted[MAXCACHE];
            memcpy(sorted, cache, sizeof(int) * (size_t) count);
            for (int i = 0; i < count; i++)
                for (int j = i + 1; j < count; j++)
                    if (sorted[j] < sorted[i]) { int tmp = sorted[i]; sorted[i] = sorted[j]; sorted[j] = tmp; }
            for (int i = 0; i < count; i++) printf("%d%s", sorted[i], (i + 1 < count) ? ", " : "");
            printf("\n");
        }
    }
    return hits;
}

/* ---------- 워크로드 생성 ---------- */

#define NREF 10000
#define UNIQ 100

static void gen_nolocality(int *t) {
    for (int i = 0; i < NREF; i++) t[i] = (int) (rng() % UNIQ);
}

static void gen_8020(int *t) {
    /* 참조의 80% 가 페이지의 20%(hot: 0~19) 로, 나머지 20% 가 80%(cold: 20~99) 로 */
    for (int i = 0; i < NREF; i++)
        t[i] = (rng() % 100 < 80) ? (int) (rng() % 20)
                                  : 20 + (int) (rng() % 80);
}

static void gen_looping(int *t) {
    /* 0..49 를 순서대로 반복 */
    for (int i = 0; i < NREF; i++) t[i] = i % 50;
}

int main(int argc, char *argv[]) {
    /* ===== 1. 원서 예제 트레이스 ===== */
    int ex[] = {0, 1, 2, 0, 1, 3, 0, 3, 1, 2, 1};
    int exn = (int) (sizeof(ex) / sizeof(ex[0]));

    printf("===== 1. 원서 예제 트레이스 (캐시 3 페이지) =====\n");
    printf("트레이스: ");
    for (int i = 0; i < exn; i++) printf("%d%s", ex[i], (i + 1 < exn) ? ", " : "\n");

    /* 강제(compulsory) 미스 = 고유 페이지 수. 각 페이지의 첫 접근은 피할 수 없다 */
    int compulsory = 0;
    for (int i = 0; i < exn; i++) {
        int seen = 0;
        for (int j = 0; j < i; j++) if (ex[j] == ex[i]) { seen = 1; break; }
        if (!seen) compulsory++;
    }
    printf("고유 페이지 %d개 → 강제 미스 %d회\n", compulsory, compulsory);

    for (policy_t p = OPT; p <= RAND; p++) {
        rng_state = 12345;                     /* RAND 재현성 */
        printf("\n--- %s ---\n", pname[p]);
        int hits = simulate(ex, exn, 3, p, 1);
        int miss = exn - hits;
        printf("  히트 %d / 미스 %d → 히트율 %.1f%%  (강제 미스 제외 시 %.1f%%)\n",
               hits, miss, 100.0 * hits / exn, 100.0 * hits / (exn - compulsory));
    }

    /* ===== 1-B. Belady 의 역설 검증 (원서 ch22 ASIDE) ===== */
    printf("\n\n===== 1-B. Belady 의 역설 (FIFO, 캐시 3 vs 4) =====\n");
    int bel[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int beln = (int) (sizeof(bel) / sizeof(bel[0]));
    printf("트레이스: ");
    for (int i = 0; i < beln; i++) printf("%d%s", bel[i], (i + 1 < beln) ? ", " : "\n");
    printf("%8s %8s %8s %8s\n", "캐시", "정책", "히트", "히트율");
    for (int cs = 3; cs <= 4; cs++) {
        for (policy_t p = FIFO; p <= LRU; p++) {
            rng_state = 12345;
            int h = simulate(bel, beln, cs, p, 0);
            printf("%8d %8s %8d %7.1f%%\n", cs, pname[p], h, 100.0 * h / beln);
        }
    }
    printf("→ FIFO 는 캐시를 3에서 4로 늘렸을 때 히트율이 오히려 떨어진다 (Belady 의 역설).\n");
    printf("  LRU 는 스택 속성(크기 N+1 캐시가 크기 N 캐시 내용을 포함)을 가져 역설이 없다.\n");

    /* ===== 2. 워크로드 3종 × 캐시 크기 ===== */
    static int trace[NREF];
    const char *wname[] = { "지역성 없음 (무작위 100 페이지)",
                            "80-20 (참조 80% 가 페이지 20% 로)",
                            "순환 순차 (0~49 반복)" };
    void (*gen[])(int *) = { gen_nolocality, gen_8020, gen_looping };
    int sizes[] = { 1, 2, 5, 10, 20, 30, 40, 50, 60, 80, 100 };
    int nsizes = (int) (sizeof(sizes) / sizeof(sizes[0]));

    for (int w = 0; w < 3; w++) {
        printf("\n\n===== 2-%d. 워크로드: %s =====\n", w + 1, wname[w]);
        rng_state = 999;                       /* 워크로드 생성 시드 고정 */
        gen[w](trace);

        printf("%8s", "캐시");
        for (policy_t p = OPT; p <= RAND; p++) printf(" %8s", pname[p]);
        printf("\n%8s", "-----");
        for (policy_t p = OPT; p <= RAND; p++) printf(" %8s", "-------");
        printf("\n");

        for (int s = 0; s < nsizes; s++) {
            printf("%8d", sizes[s]);
            for (policy_t p = OPT; p <= RAND; p++) {
                rng_state = 12345;             /* 정책 간 공정 비교 */
                int hits = simulate(trace, NREF, sizes[s], p, 0);
                printf(" %7.1f%%", 100.0 * hits / NREF);
            }
            printf("\n");
        }
    }
    return 0;
}
