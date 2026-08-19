/*
 * p1.c — fork() 만 호출 (OSTEP ch5, Figure 5.1)
 *
 * 관찰 목표
 *   1. fork() 가 호출 프로세스의 (거의) 정확한 복사본을 만든다
 *   2. 자식은 main() 부터가 아니라 fork() 에서 돌아오는 지점부터 실행된다
 *      → "hello world" 가 한 번만 출력되는 것이 그 증거
 *   3. 같은 fork() 호출이 부모에게는 자식 PID, 자식에게는 0 을 반환한다
 *   4. 출력 순서가 비결정적이다 (스케줄러가 결정)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   // fork, getpid

int main(int argc, char *argv[])
{
    printf("hello world (pid:%d)\n", (int) getpid());

    int rc = fork();   // 여기서 프로세스가 둘로 나뉜다

    if (rc < 0) {
        // fork 실패 — 프로세스 수 한계 초과 등
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // 반환값 0 → 자식(child). 새로 생긴 프로세스
        printf("hello, I am child (pid:%d)\n", (int) getpid());
    } else {
        // 반환값 > 0 → 부모(parent). rc 는 자식의 PID
        printf("hello, I am parent of %d (pid:%d)\n", rc, (int) getpid());
    }

    return 0;   // 부모·자식 모두 이 지점을 통과한다
}
