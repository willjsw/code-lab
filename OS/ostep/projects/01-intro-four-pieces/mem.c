/*
 * mem.c — 메모리 가상화 관찰용 프로그램 (OSTEP ch2, Figure 2.3)
 *
 * 하는 일: 힙에 int 한 개를 할당하고 그 주소를 출력한 뒤,
 *          1초마다 그 주소의 값을 1 증가시켜 PID 와 함께 출력.
 * 목적: 여러 개를 동시에 실행해 각 프로세스가 자기만의
 *       가상 주소 공간을 가진다는 사실을 확인.
 *
 * 종료: Control-c (SIGINT)
 */

#include <unistd.h>   // getpid
#include <stdio.h>
#include <stdlib.h>   // malloc, atoi, exit
#include "common.h"   // Spin(), assert (간접 포함)

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: mem <value>\n");
        exit(1);
    }

    int *p;
    p = malloc(sizeof(int));   // 힙에 int 크기(보통 4바이트) 할당
                               // Java 의 new 와 달리 GC 부재 → 원칙적으로 free 필요.
                               // 이 프로그램은 무한 루프 후 강제 종료되므로 생략
                               // (프로세스 종료 시 OS 가 주소 공간 전체 회수)
    assert(p != NULL);         // malloc 실패 시 NULL 반환 → 반드시 검사

    // %p — 포인터 값(주소)을 16진수로 출력
    // getpid() 반환형은 pid_t 이므로 %d 로 넘기려면 int 캐스팅
    printf("(%d) addr pointed to by p: %p\n", (int) getpid(), p);

    *p = atoi(argv[1]);        // 역참조 후 대입. p 가 아니라 p 가 가리키는 곳에 저장

    while (1) {
        Spin(1);
        *p = *p + 1;           // 매초 같은 주소의 값을 증가
        printf("(%d) value of p: %d\n", getpid(), *p);
    }

    return 0;   // 도달 불가
}
