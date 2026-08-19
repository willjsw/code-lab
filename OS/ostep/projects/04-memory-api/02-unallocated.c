/* 02-unallocated.c — 할당을 잊음 (원서 ch14 "Forgetting To Allocate Memory")
 * strcpy 는 목적지 메모리가 이미 확보되어 있다고 가정한다 */
#include <stdio.h>
#include <string.h>
int main(void) {
    char *src = "hello";
    char *dst;              // oops! 미할당 — 쓰레기 주소
    strcpy(dst, src);       // ← 세그폴트
    printf("%s\n", dst);
    return 0;
}
