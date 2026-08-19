/* 03-too-small.c — 할당 크기 부족 = 버퍼 오버플로
 * (원서 ch14 "Not Allocating Enough Memory")
 * 문자열은 종료 문자 '\0' 자리가 필요하므로 strlen(s) + 1 이 정답 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char *src = "hello";
    char *dst = (char *) malloc(strlen(src));   // ← 1바이트 부족
    strcpy(dst, src);                           // 경계 밖 1바이트 기록
    printf("%s\n", dst);
    free(dst);
    return 0;
}
