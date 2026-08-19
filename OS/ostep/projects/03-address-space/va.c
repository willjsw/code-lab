/*
 * va.c — 주소 공간 배치 관찰 (OSTEP ch13, ASIDE "Every Address You See Is Virtual")
 *
 * 핵심 명제: 사용자 프로그램이 볼 수 있는 모든 주소는 가상 주소(virtual address).
 *            물리 주소는 OS와 하드웨어만 알고 있음.
 *
 * 관찰 목표: 코드 · 힙 · 스택이 주소 공간의 어디에 놓이는지
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // main 은 함수 이름 → 함수 포인터로 감쇠(decay). 코드 세그먼트 주소
    printf("location of code : %p\n", main);

    // malloc 반환값 = 힙 주소. 100e6 은 double 리터럴(1억)이므로 size_t 로 변환됨
    printf("location of heap : %p\n", malloc(100e6));

    int x = 3;                    // 지역 변수 → 스택
    printf("location of stack: %p\n", &x);

    return 0;
}
