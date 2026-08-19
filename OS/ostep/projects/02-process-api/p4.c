/*
 * p4.c — 출력 리다이렉션 (OSTEP ch5, Figure 5.4)
 *
 * 관찰 목표
 *   셸이 `wc p4.c > p4.output` 을 구현하는 원리.
 *   fork() 와 exec() 사이의 틈에서 자식의 환경을 바꿀 수 있다는 점이 핵심.
 *
 * 동작 원리
 *   1. close(STDOUT_FILENO) — 표준 출력(fd 1) 을 닫는다
 *   2. open("./p4.output", ...) — UNIX 는 비어 있는 가장 작은 fd 번호를
 *      배정하므로, 방금 비운 1 번이 이 파일에 배정된다
 *   3. exec() 후에도 열린 파일 디스크립터는 유지된다
 *      → wc 의 printf 출력이 화면 대신 파일로 흘러간다
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // close, STDOUT_FILENO
#include <string.h>
#include <fcntl.h>      // open, O_* 플래그
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int rc = fork();

    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // 자식: 표준 출력을 파일로 돌린다
        close(STDOUT_FILENO);   // fd 1 해제 → 다음 open 이 1 번을 받는다
        open("./p4.output", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        // S_IRWXU = 소유자 읽기·쓰기·실행 (0700)

        char *myargs[3];
        myargs[0] = strdup("wc");
        myargs[1] = strdup("p4.c");
        myargs[2] = NULL;
        execvp(myargs[0], myargs);
    } else {
        int wc = wait(NULL);
        assert(wc >= 0);   // wait 실패 검증
    }

    return 0;
}
