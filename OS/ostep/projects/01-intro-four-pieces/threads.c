/*
 * threads.c — 병행성 문제 관찰용 프로그램 (OSTEP ch2, Figure 2.5)
 *
 * 하는 일: 두 스레드가 공유 변수 counter 를 각각 loops 회 증가.
 * 기대값: 2 * loops
 * 실제:   loops 가 커지면 기대값에 미달하고 실행마다 값이 달라짐
 * 원인:   counter++ 가 기계어 3개(load / increment / store)로 분해되어
 *         원자적으로 실행되지 않음 → 두 스레드 명령어가 섞이면 증가분 소실
 */

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "common_threads.h"   // Pthread_create / Pthread_join 래퍼

volatile int counter = 0;   // 두 스레드가 공유. volatile = 매번 메모리에서 다시 읽기
                            // 주의: volatile 은 최적화만 막을 뿐 원자성을 주지 않음
int loops;                  // 전역 변수 — 두 스레드가 함께 읽음

/*
 * 스레드 시작 함수.
 * 시그니처가 void *(*)(void *) 로 고정 — pthread_create 요구 형식.
 * 인자·반환값을 void* 로 주고받아 임의 타입을 전달하는 C 관용.
 */
void *worker(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
        counter++;   // ← 문제 지점. 세 명령어로 분해되는 비원자 연산
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: threads <loops>\n");
        exit(1);
    }
    loops = atoi(argv[1]);

    pthread_t p1, p2;   // 스레드 식별자. Java 의 Thread 객체 참조에 대응

    printf("Initial value : %d\n", counter);

    Pthread_create(&p1, NULL, worker, NULL);   // 인자: 식별자 주소, 속성, 시작 함수, 인자
    Pthread_create(&p2, NULL, worker, NULL);   // 속성 NULL = 기본값

    Pthread_join(p1, NULL);   // 종료 대기. Java 의 Thread#join 과 동일 개념
    Pthread_join(p2, NULL);   // 두 번째 인자는 반환값 수령 포인터. 불필요하므로 NULL

    printf("Final value   : %d\n", counter);
    return 0;
}
