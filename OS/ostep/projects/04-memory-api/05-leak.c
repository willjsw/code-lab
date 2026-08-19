/* 05-leak.c — free 를 잊음 = 메모리 누수 (원서 ch14 숙제 4)
 * 단기 실행 프로그램에서는 종료 시 OS가 주소 공간 전체를 회수하므로
 * 실질 문제가 없으나, 장기 실행 서버·커널에서는 치명적 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    for (int i = 0; i < 3; i++) {
        int *leaked = (int *) malloc(1024 * sizeof(int));   // 4KB × 3
        leaked[0] = i;
        printf("할당 #%d: %p (값 %d)\n", i, (void *) leaked, leaked[0]);
        // free(leaked);   ← 누락
    }
    return 0;
}
