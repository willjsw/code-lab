/* 07-use-after-free.c — 해제 후 사용 = 댕글링 포인터 (원서 ch14 숙제 6)
 * free 는 포인터 변수를 NULL 로 만들지 않는다. 값은 그대로 남는다 */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *data = (int *) malloc(100 * sizeof(int));
    data[0] = 42;
    free(data);                        // 여기서 해제
    printf("data[0] = %d\n", data[0]); // ← 해제 후 읽기
    return 0;
}
