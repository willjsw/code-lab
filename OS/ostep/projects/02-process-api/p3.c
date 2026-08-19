/*
 * p3.c — fork() + exec() + wait() (OSTEP ch5, Figure 5.3)
 *
 * 관찰 목표
 *   exec() 는 새 프로세스를 만들지 않는다.
 *   현재 실행 중인 프로그램(p3)을 다른 프로그램(wc)으로 "변신"시킨다.
 *   → 코드·정적 데이터를 덮어쓰고 힙·스택을 재초기화
 *   → 성공한 exec() 는 절대 반환하지 않는다
 *      (아래 "this shouldn't print out" 이 출력되지 않는 것이 증거)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>     // strdup
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    printf("hello world (pid:%d)\n", (int) getpid());

    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello, I am child (pid:%d)\n", (int) getpid());

        /*
         * execvp 인자 배열 규약
         *   myargs[0] = 프로그램 이름 (관례상 실행 파일명과 동일)
         *   myargs[1..] = 인자들
         *   마지막 = NULL — 배열 끝 표시. C 배열은 길이를 모르므로 필수
         */
        char *myargs[3];
        myargs[0] = strdup("wc");     // 실행할 프로그램: wc (단어 수 세기)
        myargs[1] = strdup("p3.c");   // 인자: 셀 대상 파일
        myargs[2] = NULL;             // 배열 끝 표시

        execvp(myargs[0], myargs);    // v = 벡터(배열)로 인자 전달
                                      // p = PATH 를 검색해 실행 파일 탐색
        printf("this shouldn't print out");   // exec 성공 시 도달 불가
    } else {
        int wc = wait(NULL);
        printf("hello, I am parent of %d (wc:%d) (pid:%d)\n",
               rc, wc, (int) getpid());
    }

    return 0;
}
