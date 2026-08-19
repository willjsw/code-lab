/*
 * layout.c — 주소 공간 세그먼트 전체를 한 번에 관찰 (확장 실습)
 *
 * va.c 가 코드·힙·스택 3곳만 보여주는 것을 세그먼트별로 확장.
 * 관찰 목표
 *   1. 각 세그먼트가 주소 공간에서 어느 방향에 놓이는지
 *   2. 힙은 주소가 커지는 방향, 스택은 작아지는 방향으로 자라는지
 *   3. 초기화 전역(data)과 미초기화 전역(bss)의 위치 관계
 */

#include <stdio.h>
#include <stdlib.h>

int    global_init   = 42;      // 초기화된 전역 → data 세그먼트
int    global_uninit;           // 미초기화 전역 → bss 세그먼트 (0 으로 초기화됨)
static int static_var = 7;      // 파일 스코프 정적 변수 → data
const  char *literal = "hello"; // 문자열 리터럴 자체는 읽기 전용 영역

void some_function(void) { }     // 또 다른 코드 세그먼트 심볼

/*
 * 재귀 호출로 스택 성장 방향을 실증.
 * 호출이 깊어질 때 프레임 주소가 커지는지 작아지는지 관찰.
 */
void stack_direction(int depth) {
    int frame_local;             // 이 호출의 스택 프레임에 놓이는 변수
    printf("  깊이 %d 프레임    : %p\n", depth, (void *) &frame_local);
    if (depth < 3)
        stack_direction(depth + 1);
}

int main(int argc, char *argv[]) {
    int    local1 = 1;          // 스택
    int    local2 = 2;          // 스택 (local1 과의 주소 차이 관찰)
    void  *h1 = malloc(16);     // 힙
    void  *h2 = malloc(16);     // 힙 (h1 과의 주소 차이 관찰)

    printf("=== 코드 (텍스트) ===\n");
    printf("  main            : %p\n", (void *) main);
    printf("  some_function   : %p\n", (void *) some_function);
    printf("=== 읽기 전용 · 데이터 ===\n");
    printf("  문자열 리터럴   : %p\n", (void *) literal);
    printf("  global_init     : %p\n", (void *) &global_init);
    printf("  static_var      : %p\n", (void *) &static_var);
    printf("  global_uninit   : %p  (값 %d)\n", (void *) &global_uninit, global_uninit);
    printf("=== 힙 (아래로 = 주소 증가 방향으로 성장) ===\n");
    printf("  malloc #1       : %p\n", h1);
    printf("  malloc #2       : %p  (차이 %ld 바이트)\n", h2, (long) ((char *) h2 - (char *) h1));
    printf("=== 스택 (주소 감소 방향으로 성장) ===\n");
    printf("  local1          : %p\n", (void *) &local1);
    printf("  local2          : %p  (차이 %ld 바이트)\n", (void *) &local2,
           (long) ((char *) &local2 - (char *) &local1));
    printf("  argv            : %p\n", (void *) argv);
    printf("=== 스택 성장 방향 (재귀 3단계) ===\n");
    stack_direction(1);

    free(h1);
    free(h2);
    return 0;
}
