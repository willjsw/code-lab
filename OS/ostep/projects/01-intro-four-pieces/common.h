#ifndef __common_h__
#define __common_h__

/*
 * common.h — 원서 전 챕터 공용 시간 측정 유틸리티.
 *
 * 헤더 가드(#ifndef ... #define ... #endif)로 중복 포함 방지.
 * Java 의 import 와 달리 C 의 #include 는 단순 텍스트 삽입이므로
 * 같은 헤더를 두 번 포함하면 함수 중복 정의 에러 발생 → 가드 필수.
 */

#include <sys/time.h>   // gettimeofday, struct timeval
#include <sys/stat.h>   // 파일 권한 매크로 (io.c 에서 사용)
#include <assert.h>     // assert — mem.c 등이 이 헤더를 통해 간접 사용

/* 현재 시각을 초 단위 실수로 반환 */
double GetTime() {
    struct timeval t;
    int rc = gettimeofday(&t, NULL);   // 두 번째 인자는 타임존, 현재는 미사용
    assert(rc == 0);                   // 실패 시 즉시 중단
    // tv_sec(초) + tv_usec(마이크로초)/1e6 → 소수점 초
    return (double) t.tv_sec + (double) t.tv_usec / 1e6;
}

/* howlong 초 동안 바쁜 대기(busy-wait). sleep 과 달리 CPU 를 계속 소모 */
void Spin(int howlong) {
    double t = GetTime();
    while ((GetTime() - t) < (double) howlong)
        ;   // 빈 문장 — 루프 본문 없음. 세미콜론 위치 주의
}

#endif // __common_h__
