/*
 * p2.c — fork() + wait() (OSTEP ch5, Figure 5.2)
 *
 * 관찰 목표
 *   wait() 를 추가하면 출력이 결정적(deterministic)이 된다.
 *   이유: 부모가 먼저 실행되더라도 wait() 에서 블록되어
 *         자식이 끝나기까지 진행하지 못한다
 *         → 자식 메시지가 항상 먼저 출력된다
 *
 * 주의: 원서 각주 — wait() 가 자식 종료 전에 반환하는 경우도 있음.
 *       man page 참조. "항상"이라는 단정에는 늘 예외가 있음
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>   // wait

int main(int argc, char *argv[])
{
    printf("hello world (pid:%d)\n", (int) getpid());

    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("hello, I am child (pid:%d)\n", (int) getpid());
        sleep(1);   // 자식을 일부러 늦춤. wait() 효과를 뚜렷하게 만들기 위함
    } else {
        int wc = wait(NULL);   // 자식 종료까지 블록. 반환값 = 종료한 자식 PID
                               // 인자 NULL = 종료 상태를 받지 않음
        printf("hello, I am parent of %d (wc:%d) (pid:%d)\n",
               rc, wc, (int) getpid());
    }

    return 0;
}
