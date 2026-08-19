/* 06-overflow.c — 배열 경계 밖 접근 (원서 ch14 숙제 5)
 * 크기 100 배열의 유효 인덱스는 0..99. data[100] 은 경계 밖 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *data = (int *) malloc(100 * sizeof(int));
    data[100] = 0;          // ← 경계 밖 1칸 기록 (heap-buffer-overflow)
    printf("data[100] = %d\n", data[100]);
    free(data);
    return 0;
}
