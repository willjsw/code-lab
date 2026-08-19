/* 08-double-free.c — 이중 해제 (원서 ch14 "Freeing Memory Repeatedly")
 * 결과는 정의되지 않음(undefined). 크래시가 흔한 결말 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *x = (int *) malloc(sizeof(int));
    *x = 1;
    free(x);
    free(x);       // ← 이중 해제
    printf("여기까지 도달할까?\n");
    return 0;
}
