/* 04-uninitialized.c — 초기화를 잊음 (원서 ch14 "Forgetting To Initialize")
 * malloc 은 값을 0 으로 채우지 않는다. calloc 은 채운다 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *x = (int *) malloc(sizeof(int));
    printf("초기화 전 값: %d\n", *x);   // ← 미초기화 읽기 (값 불확정)
    int *y = (int *) calloc(1, sizeof(int));
    printf("calloc 값   : %d\n", *y);   // calloc 은 0 보장
    free(x);
    free(y);
    return 0;
}
