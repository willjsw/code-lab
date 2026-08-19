/*
 * io.c — 영속성 관찰용 프로그램 (OSTEP ch2, Figure 2.6)
 *
 * 하는 일: /tmp/file 을 만들고 "hello world\n" 를 기록.
 * 목적: 파일 시스템으로 향하는 시스템 콜 3개(open / write / close)를 확인.
 *       프로세스가 끝난 뒤에도 데이터가 남는다는 것이 영속성.
 */

#include <stdio.h>
#include <unistd.h>     // write, close, fsync
#include <assert.h>
#include <fcntl.h>      // open, O_* 플래그
#include <sys/stat.h>   // S_IRUSR, S_IWUSR 권한 매크로
#include <sys/types.h>
#include <string.h>     // strlen

int main(int argc, char *argv[]) {
    /*
     * open 인자
     *   경로   — "/tmp/file"
     *   플래그 — O_WRONLY 쓰기 전용 | O_CREAT 없으면 생성 | O_TRUNC 있으면 길이 0 으로 절단
     *   모드   — S_IRUSR|S_IWUSR = 0600, 소유자만 읽기·쓰기
     * 반환값 — 파일 디스크립터(0 이상 정수). 실패 시 -1
     */
    int fd = open("/tmp/file", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    assert(fd >= 0);

    char buffer[20];                      // 스택 배열. 힙 아님 → free 불필요
    sprintf(buffer, "hello world\n");     // 버퍼에 문자열 구성

    // write(fd, 버퍼, 바이트 수) → 반환값은 실제 기록된 바이트 수
    int rc = write(fd, buffer, strlen(buffer));
    assert(rc == (int) strlen(buffer));   // 부분 쓰기 발생 여부 검증

    fsync(fd);    // 파일 시스템이 성능을 위해 쓰기를 지연시키므로,
                  // 저장 장치까지 강제 반영하려면 명시 호출 필요
    close(fd);    // 디스크립터 해제. 더 쓰지 않음을 OS 에 통보

    return 0;
}
