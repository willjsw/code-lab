/*
 * cpu.c — CPU 가상화 관찰용 프로그램 (OSTEP ch2, Figure 2.1)
 *
 * 하는 일: 1초마다 인자로 받은 문자열을 무한 출력.
 * 목적: 여러 개를 동시에 실행해 "물리 CPU 는 적은데 여러 프로그램이
 *       동시에 도는 것처럼 보이는 환상"을 눈으로 확인.
 *
 * 종료: Control-c (SIGINT)
 */

#include <stdio.h>
#include <stdlib.h>   // exit, atoi
#include "common.h"   // Spin() — 1초 바쁜 대기

int main(int argc, char *argv[])
{
    // argc = 인자 개수(프로그램 이름 포함). argv[0] 은 프로그램 경로
    // 문자열 1개를 받아야 하므로 argc 는 정확히 2 여야 함
    if (argc != 2) {
        fprintf(stderr, "usage: cpu <string>\n");   // 사용법은 표준 에러로
        exit(1);                                    // 0 이 아닌 종료 코드 = 실패
    }

    char *str = argv[1];   // 복사 아님. argv 메모리를 가리키는 포인터일 뿐

    while (1) {            // 무한 루프. C 에는 Java 의 while(true) 대신 while(1) 관용
        printf("%s\n", str);
        Spin(1);           // sleep 이 아니라 바쁜 대기 → CPU 를 계속 점유
    }

    return 0;   // 도달 불가. 형식상 유지
}
