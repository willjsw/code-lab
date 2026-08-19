/* 01-null.c — NULL 역참조 (원서 ch14 숙제 1)
 * 기대: 세그멘테이션 폴트 */
#include <stdio.h>
int main(void) {
    int *p = NULL;
    printf("%d\n", *p);   // ← NULL 역참조
    return 0;
}
