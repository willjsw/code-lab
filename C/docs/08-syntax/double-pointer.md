---
tags:
  - lang/c
  - c/syntax
  - pointer
  - double-pointer
  - out-parameter
  - linked-list
  - status/verified
aliases:
  - 이중 포인터
  - 포인터의 포인터
  - char **
created: 2026-08-18
updated: 2026-08-18
---

# 이중 포인터 `**`

> **포인터 변수 자체를 바꿔야 할 때** 필요. C에 참조 전달이 없어 생기는 유일한 우회로

## 개념

`int **pp` — `int *`를 담는 포인터. 한 단계 더 들어가는 것이 아니라 **가리키는 대상이 포인터**

| 표기 | 담는 값 | 역참조 결과 |
|---|---|---|
| `int v` | 정수 | — |
| `int *p` | `v`의 주소 | `*p` = 정수 |
| `int **pp` | `p`의 주소 | `*pp` = 포인터, `**pp` = 정수 |

핵심 질문 — **무엇을 바꾸려 하는가**

- 가리키는 **값**을 바꿈 → 단일 포인터(`int *`)로 충분
- **포인터 변수 자체**를 바꿈(할당·재할당·재연결) → 이중 포인터 필요

## 주소 관계

```c
#include <stdio.h>

int main(void) {
    int   v  = 42;
    int  *p  = &v;
    int **pp = &p;

    printf("v   = %d\n", v);
    printf("&v  = %p   (변수 v의 주소)\n", (void *)&v);
    printf("p   = %p   (p가 담은 값 = v의 주소)\n", (void *)p);
    printf("&p  = %p   (변수 p 자신의 주소)\n", (void *)&p);
    printf("pp  = %p   (pp가 담은 값 = p의 주소)\n", (void *)pp);
    printf("&pp = %p   (변수 pp 자신의 주소)\n", (void *)&pp);
    printf("\n");
    printf("*p   = %d      (p가 가리키는 곳의 값)\n", *p);
    printf("*pp  = %p   (pp가 가리키는 곳의 값 = p)\n", (void *)*pp);
    printf("**pp = %d      (두 번 따라가면 v)\n", **pp);

    **pp = 99;                      /* 두 단계 거쳐 v 변경 */
    printf("\n**pp = 99 대입 후 v = %d\n", v);

    printf("\nsizeof: int=%zu  int*=%zu  int**=%zu\n",
           sizeof(int), sizeof(int *), sizeof(int **));
    return 0;
}
```

```bash
cc -Wall -Wextra addr.c -o addr && ./addr
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o addr` — 출력 파일명을 `addr`로 지정. 미지정 시 `a.out`
- `&& ./addr` — 컴파일 성공 시에만 실행

```
v   = 42
&v  = 0x16b0667e8   (변수 v의 주소)
p   = 0x16b0667e8   (p가 담은 값 = v의 주소)
&p  = 0x16b0667e0   (변수 p 자신의 주소)
pp  = 0x16b0667e0   (pp가 담은 값 = p의 주소)
&pp = 0x16b0667d8   (변수 pp 자신의 주소)

*p   = 42      (p가 가리키는 곳의 값)
*pp  = 0x16b0667e8   (pp가 가리키는 곳의 값 = p)
**pp = 42      (두 번 따라가면 v)

**pp = 99 대입 후 v = 99

sizeof: int=4  int*=8  int**=8
```

- `p == &v`, `pp == &p` — 각 단계가 **앞 변수의 주소** 보관
- `**pp = 99`로 두 단계 거쳐 `v` 변경 확인
- `sizeof(int *)` = `sizeof(int **)` = **8** — 포인터는 몇 겹이든 주소 크기 동일. 단계 수는 **타입 정보일 뿐**

```mermaid
flowchart LR
    pp["int **pp<br/>0x16b0667d8<br/>값 = 0x16b0667e0"]
    p["int *p<br/>0x16b0667e0<br/>값 = 0x16b0667e8"]
    v["int v<br/>0x16b0667e8<br/>값 = 42"]

    pp -->|"*pp"| p
    p -->|"*p"| v
    pp -.->|"**pp — 두 단계"| v

    classDef ptr fill:#e0f0ff,stroke:#06c
    classDef val fill:#e0ffe0,stroke:#0a0
    class pp,p ptr
    class v val
```

## 용법 1 — 함수에서 포인터 자체 변경

가장 중요한 용도. C는 **전부 값 전달**이므로 포인터를 넘겨도 그 사본이 전달됨

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 단일 포인터 — 호출자의 포인터를 바꾸지 못함 */
static void alloc_bad(char *p) {
    p = malloc(16);
    strcpy(p, "BAD");
}

/* 이중 포인터 — 호출자의 포인터 자체를 변경 */
static void alloc_good(char **pp) {
    *pp = malloc(16);
    strcpy(*pp, "GOOD");
}

int main(void) {
    char *a = NULL;
    alloc_bad(a);
    printf("alloc_bad  후: a = %p (%s)\n", (void *)a, a ? a : "NULL 그대로");

    char *b = NULL;
    alloc_good(&b);
    printf("alloc_good 후: b = %p (%s)\n", (void *)b, b ? b : "NULL");

    free(b);
    return 0;
}
```

```bash
cc -Wall -Wextra why.c -o why && ./why
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o why` — 출력 파일명을 `why`로 지정. 미지정 시 `a.out`
- `&& ./why` — 컴파일 성공 시에만 실행

```
alloc_bad  후: a = 0x0 (NULL 그대로)
alloc_good 후: b = 0x104f899a0 (GOOD)
```

- `alloc_bad` — 매개변수 `p`는 `a`의 **사본**. `p = malloc(...)`은 사본만 변경 → `a`는 `NULL` 유지
- 부작용 — 할당한 16바이트가 **누수**. 아무도 그 주소를 모름
- `alloc_good` — `&b`로 **`b` 자신의 주소** 전달 → `*pp = ...`가 `b`를 직접 변경

판단 기준

| 함수가 하려는 일 | 필요한 타입 |
|---|---|
| 가리키는 배열·구조체 **내용** 수정 | `T *` |
| 포인터에 **새 주소 대입**(할당·재할당·재연결) | `T **` |

## 용법 2 — 문자열 배열 `char **argv`

`char *`들의 배열. 배열 이름이 함수 인자로 전달될 때 `char **`로 감쇠

```c
#include <stdio.h>

static void print_all(char **argv, int argc) {
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = %p → \"%s\"\n", i, (void *)argv[i], argv[i]);
}

/* NULL 종단 규약 — 개수를 안 넘겨도 순회 가능 */
static void print_until_null(char **argv) {
    for (char **p = argv; *p != NULL; p++)
        printf("  %s", *p);
    printf("\n");
}

int main(void) {
    char *words[] = { "ls", "-l", "/tmp", NULL };   /* 배열 이름 → char ** 감쇠 */

    printf("words 배열 자체 주소 = %p\n", (void *)words);
    printf("sizeof(words) = %zu (포인터 4개)\n\n", sizeof(words));

    print_all(words, 3);
    printf("\nNULL 종단 순회:");
    print_until_null(words);

    printf("\n포인터 산술: *(words+1) = \"%s\", words[1] = \"%s\"\n",
           *(words + 1), words[1]);
    return 0;
}
```

```bash
cc -Wall -Wextra argv.c -o argv && ./argv
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o argv` — 출력 파일명을 `argv`로 지정. 미지정 시 `a.out`
- `&& ./argv` — 컴파일 성공 시에만 실행

```
words 배열 자체 주소 = 0x16d5127e0
sizeof(words) = 32 (포인터 4개)

  argv[0] = 0x1028ec6a4 → "ls"
  argv[1] = 0x1028ec6a7 → "-l"
  argv[2] = 0x1028ec6aa → "/tmp"

NULL 종단 순회:  ls  -l  /tmp

포인터 산술: *(words+1) = "-l", words[1] = "-l"
```

- `sizeof(words)` = 32 = 포인터 8바이트 × 4개(`NULL` 포함). **문자열 길이 총합과 무관**
- 문자열 주소 `…6a4`·`…6a7`·`…6aa` — 읽기 전용 영역에 연속 배치. `"ls\0"` 3바이트씩 간격
- `argv[i]`는 `*(argv + i)`와 동등 — 배열 첨자가 포인터 산술의 문법 설탕
- **`NULL` 종단** — 개수 인자 없이 순회 가능. `execvp`·`main(int argc, char **argv)` 규약

```mermaid
flowchart LR
    subgraph Arr["char *words[4] — 스택, 32바이트"]
        a0["words[0]<br/>0x1028ec6a4"]
        a1["words[1]<br/>0x1028ec6a7"]
        a2["words[2]<br/>0x1028ec6aa"]
        a3["words[3]<br/>NULL — 종단"]
    end
    subgraph RO["읽기 전용 영역 — 문자열 실체"]
        s0["'l' 's' 널<br/>3바이트"]
        s1["'-' 'l' 널<br/>3바이트"]
        s2["'/tmp' 널<br/>5바이트"]
    end
    a0 --> s0
    a1 --> s1
    a2 --> s2

    classDef arr fill:#e0f0ff,stroke:#06c
    classDef ro fill:#f0f0f0,stroke:#888
    classDef nul fill:#ffe0e0,stroke:#c00
    class a0,a1,a2 arr
    class s0,s1,s2 ro
    class a3 nul
```

배열은 **주소만** 보관. 문자열 실체는 별도 영역 — 2단계 구조가 이중 포인터가 필요한 이유

## 용법 3 — `realloc` 성장 버퍼

`realloc`이 **주소를 옮길 수 있으므로** 호출자 포인터 갱신 필수. `make-shell`의 `read_line`이 이 구조

```c
#include <stdio.h>
#include <stdlib.h>

/* realloc은 주소를 옮길 수 있음 → 호출자 포인터 갱신 위해 ** 필요 */
static int push_char(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 1 >= *cap) {
        size_t ncap = *cap * 2;
        char *tmp = realloc(*buf, ncap);
        if (tmp == NULL) return -1;          /* 원본 유지 → 호출자가 free */
        *buf = tmp;                          /* ← 호출자 포인터 갱신 */
        *cap = ncap;
        printf("  [확장] cap %zu → %zu, 주소 %p\n", ncap / 2, ncap, (void *)*buf);
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
    return 0;
}

int main(void) {
    size_t cap = 4, len = 0;
    char *buf = malloc(cap);
    buf[0] = '\0';
    printf("초기 주소 %p (cap %zu)\n", (void *)buf, cap);

    for (char c = 'a'; c <= 'j'; c++)
        if (push_char(&buf, &len, &cap, c) != 0) { free(buf); return 1; }

    printf("결과 \"%s\" (len %zu, cap %zu)\n", buf, len, cap);
    free(buf);
    return 0;
}
```

```bash
cc -Wall -Wextra grow.c -o grow && ./grow
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o grow` — 출력 파일명을 `grow`로 지정. 미지정 시 `a.out`
- `&& ./grow` — 컴파일 성공 시에만 실행

```
초기 주소 0x1029a1ae0 (cap 4)
  [확장] cap 4 → 8, 주소 0x1029a1ae0
  [확장] cap 8 → 16, 주소 0x1029a1ae0
결과 "abcdefghij" (len 10, cap 16)
```

- `(*buf)[*len]` — **괄호 필수**. `*buf[*len]`은 `*(buf[*len])`로 해석되어 전혀 다른 동작
- `len`·`cap`도 호출자 변수를 갱신해야 하므로 `size_t *`로 전달
- `realloc` 실패 시 원본 유지 → `tmp` 경유 후 성공 시에만 `*buf` 갱신

### 주소 이동 실증 — 단일 포인터 버그가 숨는 이유

위 실행에서는 주소가 유지되어(`0x1029a1ae0` 고정) **단일 포인터로도 우연히 동작**. 확장을 방해하면 이동 발생

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    size_t cap = 8;
    char *buf = malloc(cap);
    char *block = malloc(64);        /* 뒤를 막아 in-place 확장 방해 */
    printf("최초      %p\n", (void*)buf);
    for (int i = 0; i < 6; i++) {
        cap *= 8;
        char *tmp = realloc(buf, cap);
        if (!tmp) { free(buf); free(block); return 1; }
        printf("cap %7zu %p %s\n", cap, (void*)tmp, tmp == buf ? "(제자리)" : "← 주소 이동");
        buf = tmp;
    }
    free(buf); free(block);
    return 0;
}
```

```bash
cc -Wall -Wextra move.c -o move && ./move
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o move` — 출력 파일명을 `move`로 지정. 미지정 시 `a.out`
- `&& ./move` — 컴파일 성공 시에만 실행

```
최초      0x101329ae0
cap      64 0x1013299f0 ← 주소 이동
cap     512 0x10132af40 ← 주소 이동
cap    4096 0x10132b140 ← 주소 이동
cap   32768 0xcb0c08000 ← 주소 이동
cap  262144 0xcb1000000 ← 주소 이동
cap 2097152 0xcb1000000 (제자리)
```

- 대부분 **주소 이동**. `0x1013…` → `0xcb0c…`는 힙 경로에서 `mmap` 경로로 전환된 결과
- 결론 — "작은 입력에서는 잘 되던 코드"가 입력이 커지면 붕괴. **제자리 확장은 우연**
- 이중 포인터 미사용 시 갱신 누락 → 해제된 옛 주소 참조 → use-after-free

## 용법 4 — 연결 리스트 삭제

"다음을 가리키는 자리"를 직접 다뤄 **첫 노드 특수 처리 제거**

```c
/* 단일 포인터 + prev 추적 — 분기 필요 */
static void remove_prev(Node **head, int v) {
    Node *cur = *head, *prev = NULL;
    while (cur) {
        if (cur->val == v) {
            if (prev) prev->next = cur->next;   /* 중간 노드 */
            else      *head = cur->next;        /* 첫 노드 — 별도 처리 */
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

/* 이중 포인터 — "다음을 가리키는 자리"를 직접 다룸. 분기 부재 */
static void remove_dptr(Node **head, int v) {
    for (Node **pp = head; *pp; pp = &(*pp)->next) {
        if ((*pp)->val == v) {
            Node *dead = *pp;
            *pp = dead->next;                   /* 첫 노드든 중간이든 동일 */
            free(dead);
            return;
        }
    }
}
```

```bash
cc -Wall -Wextra list.c -o list && ./list
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o list` — 출력 파일명을 `list`로 지정. 미지정 시 `a.out`
- `&& ./list` — 컴파일 성공 시에만 실행

```
초기:         1 2 3 
prev 방식:    2 3 
이중포인터:   2 3 
3 삭제 후:    2 
정리 후 a=0x0 b=0x0
```

- `pp`가 가리키는 것 — 노드가 아니라 **"노드 주소를 담은 칸"**. 처음엔 `head` 변수, 이후엔 각 노드의 `next` 필드
- 첫 노드 삭제든 중간·마지막 삭제든 `*pp = dead->next` **한 줄로 통일**
- `prev` 방식 대비 분기 1개·변수 1개 감소
- `push`도 `Node **head` 필요 — `*head`에 새 노드 주소를 대입해야 하므로

```mermaid
flowchart LR
    H["head 변수<br/>(Node* 를 담는 칸)"] --> N1["Node 1"]
    N1 -->|"next 필드<br/>(Node* 를 담는 칸)"| N2["Node 2"]
    N2 --> N3["Node 3"]

    PP["Node **pp<br/>이 '칸'들을 순회"] -.->|"pp = head"| H
    PP -.->|"pp = &N1->next"| N1

    classDef slot fill:#e0f0ff,stroke:#06c
    classDef node fill:#e0ffe0,stroke:#0a0
    class H,PP slot
    class N1,N2,N3 node
```

`head`와 `next`가 **같은 타입의 칸**(`Node *`)이라는 점이 통일의 근거

## 용법 5 — 2차원 동적 배열

행 포인터 배열 + 각 행 실체. 2단계 할당·2단계 해제

```c
int rows = 3, cols = 4;
int **m = malloc((size_t)rows * sizeof *m);      /* 행 포인터 배열 */
for (int i = 0; i < rows; i++) {
    m[i] = malloc((size_t)cols * sizeof **m);    /* 각 행 실체 */
    for (int j = 0; j < cols; j++) m[i][j] = i * 10 + j;
}
/* ... 사용 ... */
for (int i = 0; i < rows; i++) free(m[i]);       /* 역순 해제 — 행 먼저 */
free(m);
```

```
2차원 배열:
  m[0] = 0x1029a1ae0 →   0  1  2  3
  m[1] = 0x1029a19d0 →  10 11 12 13
  m[2] = 0x1029a19e0 →  20 21 22 23
```

- 각 행 주소가 **불연속** — 진짜 2차원 배열(`int m[3][4]`)과 메모리 배치 상이
- `sizeof *m` = `sizeof(int *)`, `sizeof **m` = `sizeof(int)` — 타입명 하드코딩 회피 관용 표현
- **해제 순서** — 행 먼저, 배열 나중. 역순 시 행 주소 유실 → 누수
- 연속 메모리가 필요하면 1차원 할당 후 `m[i * cols + j]` 색인 방식이 유리

## Java와의 차이

Java에 이중 포인터가 없는 이유 — **참조 재대입을 메서드가 할 수 없도록** 설계됨

| 항목 | Java | C |
|---|---|---|
| 인자 전달 | 값 전달(참조도 값으로 복사) | 값 전달 — **동일** |
| 메서드에서 인자 재대입 | 호출자에 **미반영** | 단일 포인터도 미반영 |
| 우회 수단 | 반환값·배열·wrapper 객체 | **이중 포인터** |
| 문자열 배열 | `String[]` | `char **` |
| 2차원 배열 | `int[][]` — 실체도 배열 객체 | `int **` — 수동 2단계 할당 |
| 크기 정보 | `arr.length` 내장 | **별도 전달** 또는 `NULL` 종단 |

Java에서 같은 문제를 만나는 지점

```java
void alloc(String s) { s = "GOOD"; }   // 호출자 변수 불변 — C의 alloc_bad와 동일
String alloc() { return "GOOD"; }      // 반환값으로 해결 — Java의 정석
```

- Java는 **반환값**으로 해결. C도 단순한 경우 반환값이 더 명료
- C에서 이중 포인터가 필요한 경우 — 반환값을 **오류 코드**로 쓰면서 결과도 넘겨야 할 때
  ```c
  int read_line(char **out);   /* 반환 = 성공 여부, *out = 결과 버퍼 */
  ```

## 함정 · 주의점

- 단일 포인터로 호출자 포인터 변경 시도 → 무반응 + **누수**. `&` 붙여 전달했는지 확인
- `*buf[i]` vs `(*buf)[i]` — 우선순위상 `[]`가 `*`보다 강함. **괄호 필수**
  ```c
  (*buf)[0] = 'a';    // ← 버퍼 첫 바이트
  *buf[0]   = 'a';    // ← buf[0]이 가리키는 곳. 전혀 다름
  ```
- `char **`에 `char (*)[N]` 전달 → 타입 불일치. 2차원 배열과 포인터 배열은 **별개 타입**
  ```c
  char grid[3][10];
  char **pp = grid;   // ← 컴파일 오류. 메모리 배치가 다름
  ```
- `realloc` 결과를 `*buf`에 **직접 대입** → 실패 시 원본 주소 유실. `tmp` 경유 필수
- 2차원 배열 해제 시 `free(m)` 먼저 → 행 주소 전부 유실. **행 먼저 해제**
- `NULL` 종단 배열에서 종단 원소 누락 → 순회가 배열 밖으로 진행. 할당 시 `+1` 확보
- 삼중 포인터(`***`) 등장 → 대개 설계 문제. 구조체로 묶는 편이 명료
- `void **`로 임의 포인터 타입 수용 시도 → `void *`와 달리 **자동 변환 부재**. 명시적 캐스팅 필요

## 검증

- [x] 단일 포인터 out-parameter 실패·이중 포인터 성공 확인
- [x] `p`·`pp` 주소 관계와 `**pp` 대입 반영 확인
- [x] `sizeof(int *)` = `sizeof(int **)` = 8 확인
- [x] `char *words[]` 크기 32바이트·문자열 주소 분리 확인
- [x] `realloc` 주소 이동 실증 (in-place 확장이 우연임을 확인)
- [x] 연결 리스트 첫 노드·중간 노드 동일 코드 삭제 확인
- [x] 2차원 배열 행 주소 불연속 확인
- [x] 전 예제 ASan 무오류·무누수 통과

## 관련 문서

- [[C/projects/make-shell/03-tokenizer|03 토크나이저]] — `char **argv` 구성 실전 적용
- [[C/projects/make-shell/02-dynamic-input|02 동적 입력 버퍼]] — `realloc` 성장 버퍼와 소유권 규약
- [[C/docs/08-syntax/sizeof-and-array-subscript|sizeof 연산자와 배열 첨자]] — 배열 감쇠와 포인터 산술
- [[C/docs/07-stdlib/03-stdlib|메모리 · 변환]] — `malloc`·`realloc` 사용 규칙
- [[C/docs/02-memory/heap-and-free|free의 실제 동작]] — 해제 후 주소 상태와 use-after-free
- [[C/docs/05-debugging/lldb-memory-inspection|lldb로 메모리 주소 값 조회하기]] — 포인터 관계를 디버거로 직접 확인
- [[C/docs/README|학습 문서 인덱스]] — 카테고리별 전체 문서 목록
