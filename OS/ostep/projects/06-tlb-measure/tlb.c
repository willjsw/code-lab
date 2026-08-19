/*
 * tlb.c — TLB 크기와 미스 비용 측정 (OSTEP ch19 Homework: Measurement)
 *
 * 원리 (Saavedra-Barrera 의 방법)
 *   배열을 "페이지 크기 간격"으로 건드리면 접근 1회당 페이지 1개를 만진다.
 *   만지는 페이지 수가 TLB 항목 수 이하면 전부 TLB 히트 → 빠름.
 *   TLB 용량을 넘어서면 매 접근이 TLB 미스 → 접근당 시간이 급증.
 *   → 시간이 꺾이는 지점이 TLB 용량의 경계.
 *
 * 주의
 *   - 최적화가 루프를 제거하지 못하도록 배열을 volatile 로 둔다
 *   - 페이지 크기는 실행 시점에 sysconf 로 조회 (arm64 macOS 는 16KB)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* 나노초 단위 단조 증가 시계 — gettimeofday 보다 정밀하고 시스템 시각 변경에 영향 없음 */
static double now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double) t.tv_sec * 1e9 + (double) t.tv_nsec;
}

/* 페이지 numpages 개를 페이지 간격으로 반복 접근하고 접근당 나노초를 반환 */
static double measure(int numpages, long page, int jump) {
    size_t nbytes = (size_t) numpages * (size_t) page;

    /* calloc 으로 확보 후 미리 한 번 전체를 건드려 페이지 폴트를 먼저 소진시킨다
     * (측정 대상은 TLB 미스이지 페이지 폴트가 아니다) */
    volatile int *a = calloc(nbytes, 1);
    if (!a) { fprintf(stderr, "할당 실패: %zu 바이트\n", nbytes); return -1; }
    for (int i = 0; i < numpages * jump; i += jump) a[i] = 0;

    /* 총 접근 횟수를 페이지 수와 무관하게 일정하게 유지 → 비교 가능성 확보 */
    const long TOTAL = 40L * 1000 * 1000;
    long trials = TOTAL / numpages;
    if (trials < 1) trials = 1;

    double t0 = now_ns();
    for (long t = 0; t < trials; t++)
        for (int i = 0; i < numpages * jump; i += jump)
            a[i] += 1;
    double t1 = now_ns();

    free((void *) a);
    return (t1 - t0) / ((double) trials * numpages);
}

int main(int argc, char *argv[]) {
    long page = sysconf(_SC_PAGESIZE);          // 실행 환경의 페이지 크기
    int  jump = (int) (page / sizeof(int));      // 페이지 1개를 건너뛰는 인덱스 간격

    printf("페이지 크기 = %ld 바이트, 정수 간격(jump) = %d\n", page, jump);

    /* 2단계: 8~64 구간을 1씩 늘려 경계를 정밀 탐색 */
    printf("\n[정밀 측정] 8~64 페이지, 1씩 증가\n");
    printf("%10s %12s\n", "페이지 수", "접근당(ns)");
    printf("%10s %12s\n", "--------", "---------");
    for (int n = 8; n <= 64; n++) {
        double ns = measure(n, page, jump);
        printf("%10d %12.2f%s\n", n, ns, (n % 8 == 0) ? "   <-- 8의 배수" : "");
    }

    printf("\n[개괄 측정] 1~8192 페이지, 2배씩 증가\n");
    printf("%10s %14s %14s %12s\n", "페이지 수", "총 접근 횟수", "총 시간(ms)", "접근당(ns)");
    printf("%10s %14s %14s %12s\n", "--------", "-----------", "----------", "---------");

    /* 페이지 수를 2배씩 늘려가며 측정 */
    for (int numpages = 1; numpages <= 8192; numpages *= 2) {
        size_t nbytes = (size_t) numpages * (size_t) page;

        /* calloc 으로 확보 후 미리 한 번 전체를 건드려 페이지 폴트를 먼저 소진시킨다
         * (측정 대상은 TLB 미스이지 페이지 폴트가 아니다) */
        volatile int *a = calloc(nbytes, 1);
        if (!a) { fprintf(stderr, "할당 실패: %zu 바이트\n", nbytes); break; }
        for (int i = 0; i < numpages * jump; i += jump) a[i] = 0;

        /* 총 접근 횟수를 페이지 수와 무관하게 일정하게 유지 → 비교 가능성 확보 */
        const long TOTAL = 40L * 1000 * 1000;
        long trials = TOTAL / numpages;
        if (trials < 1) trials = 1;

        double t0 = now_ns();
        for (long t = 0; t < trials; t++)
            for (int i = 0; i < numpages * jump; i += jump)
                a[i] += 1;
        double t1 = now_ns();

        double accesses = (double) trials * numpages;
        printf("%10d %14.0f %14.1f %12.2f\n",
               numpages, accesses, (t1 - t0) / 1e6, (t1 - t0) / accesses);

        free((void *) a);
    }
    return 0;
}
