/* 09-invalid-free.c — 잘못된 free (원서 ch14 "Calling free() Incorrectly")
 * free 는 malloc 이 반환한 포인터만 받아야 한다 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *x = (int *) malloc(10 * sizeof(int));
    free(x + 1);   // ← malloc 이 준 주소가 아님
    printf("여기까지 도달할까?\n");
    return 0;
}
