---
tags:
  - lang/c
  - c/stdlib
  - stdio
  - printf
  - file-io
  - stream
  - status/verified
aliases:
  - stdio.h
  - printf 서식
created: 2026-08-14
updated: 2026-08-18
---

# `<stdio.h>` — 표준 입출력

> 화면 출력·키보드 입력·파일 읽기쓰기. 서식 지정자와 `FILE *` 스트림

## 헤더 포함

```c
#include <stdio.h>
```

## 서식 출력 — `printf` 계열

| 함수 | 시그니처 | 용도 |
|---|---|---|
| `printf` | `int printf(const char *fmt, ...)` | 표준 출력 |
| `fprintf` | `int fprintf(FILE *fp, const char *fmt, ...)` | 파일·스트림 출력 |
| `sprintf` | `int sprintf(char *buf, const char *fmt, ...)` | 문자열로 출력 — **위험** |
| `snprintf` | `int snprintf(char *buf, size_t n, const char *fmt, ...)` | 크기 제한 문자열 출력 — **권장** |
| `puts` | `int puts(const char *s)` | 문자열 + 개행 출력 |
| `putchar` | `int putchar(int c)` | 문자 1개 출력 |

반환값 — 출력한 문자 수. `snprintf`는 **필요했던 전체 길이** (잘림 감지 가능)

### 서식 지정자

```c
#include <stdio.h>

int main(void) {
    printf("정수 %d | 부호없음 %u | 8진 %o | 16진 %x/%X\n", -42, 42u, 64, 255, 255);
    printf("실수 %f | 지수 %e | 짧은쪽 %g\n", 3.14159, 31415.9, 0.000123);
    printf("문자 %c | 문자열 %s | 포인터 %p\n", 'A', "hi", (void*)main);
    printf("퍼센트 %% | size_t %zu | long %ld\n", (size_t)100, 123456789L);
    printf("폭10우측[%10d] 좌측[%-10d] 0채움[%010d]\n", 42, 42, 42);
    printf("소수2자리 %.2f | 문자열4자[%.4s] | 폭*가변 %*d\n", 3.14159, "abcdefg", 6, 7);
    return 0;
}
```

```bash
cc -Wall -Wextra fmt.c -o fmt && ./fmt
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o fmt` — 출력 파일명을 `fmt`로 지정. 미지정 시 `a.out`
- `&& ./fmt` — 컴파일 성공 시에만 실행

```
정수 -42 | 부호없음 42 | 8진 100 | 16진 ff/FF
실수 3.141590 | 지수 3.141590e+04 | 짧은쪽 0.000123
문자 A | 문자열 hi | 포인터 0x100c94460
퍼센트 % | size_t 100 | long 123456789
폭10우측[        42] 좌측[42        ] 0채움[0000000042]
소수2자리 3.14 | 문자열4자[abcd] | 폭*가변      7
```

### 지정자 표

| 지정자 | 타입 | 비고 |
|---|---|---|
| `%d` `%i` | `int` | 부호 있는 10진 |
| `%u` | `unsigned int` | 부호 없는 10진 |
| `%ld` `%lu` | `long` `unsigned long` | `l` = long |
| `%lld` `%llu` | `long long` | |
| `%zu` | `size_t` | `sizeof`·`strlen` 결과에 사용 |
| `%o` `%x` `%X` | 정수 | 8진 · 16진 소문자/대문자 |
| `%f` | `double` | 기본 소수 6자리 |
| `%e` `%E` | `double` | 지수 표기 |
| `%g` `%G` | `double` | `%f`·`%e` 중 짧은 쪽 |
| `%c` | `int`(문자) | |
| `%s` | `char *` | |
| `%p` | `void *` | 주소. 캐스팅 권장 |
| `%%` | — | `%` 리터럴 |

### 서식 수식어

```
%[플래그][폭][.정밀도][길이]지정자
```

| 요소 | 예 | 효과 |
|---|---|---|
| 플래그 `-` | `%-10d` | 좌측 정렬 |
| 플래그 `0` | `%010d` | 0으로 채움 |
| 플래그 `+` | `%+d` | 양수에도 부호 |
| 폭 | `%10d` | 최소 10칸 |
| 폭 `*` | `%*d` | 폭을 인자로 전달 |
| 정밀도 `.n` | `%.2f` | 소수 2자리 |
| 정밀도(문자열) | `%.4s` | 앞 4글자만 |

### `sprintf` 대신 `snprintf`

```c
char out[64];
int n = snprintf(out, sizeof(out), "%s-%d", "id", 7);
printf("snprintf → \"%s\" (필요길이 %d)\n", out, n);
```

```
snprintf → "id-7" (필요길이 4)
```

- `sprintf` — 버퍼 크기 검사 부재 → **버퍼 오버플로**. 사용 금지 수준
- `snprintf` — 크기 초과 시 잘라내고 널 종단 보장
- 잘림 감지 — 반환값 `>= 버퍼 크기`이면 잘림 발생

## 서식 입력 — `scanf` 계열

| 함수 | 용도 | 위험도 |
|---|---|---|
| `scanf` | 표준 입력 파싱 | 높음 |
| `fscanf` | 파일에서 파싱 | 높음 |
| `sscanf` | 문자열에서 파싱 | 중간 |

```c
int a; char buf[32];
sscanf("123 hello", "%d %31s", &a, buf);
printf("sscanf → %d, %s\n", a, buf);
```

```
sscanf → 123, hello
```

- `%31s` — **폭 지정 필수**. `%s`만 쓰면 버퍼 크기 무시 → 오버플로
- 폭은 버퍼 크기 - 1 (널 종단 자리)
- 인자에 **`&` 필수** (문자열 배열 제외)
- 반환값 — 성공적으로 읽은 항목 수. 검사 권장

**권장 패턴** — `scanf` 대신 `fgets` + `sscanf`

```c
char line[256];
if (fgets(line, sizeof(line), stdin) != NULL) {
    int n;
    if (sscanf(line, "%d", &n) == 1) {
        // 정상 처리
    }
}
```

- `scanf("%d")`는 입력 실패 시 스트림에 잘못된 문자가 남아 무한 루프 유발

## 문자·문자열 입력

| 함수 | 시그니처 | 비고 |
|---|---|---|
| `fgets` | `char *fgets(char *s, int n, FILE *fp)` | **권장**. 개행 포함, 크기 제한 |
| `fgetc` | `int fgetc(FILE *fp)` | 문자 1개. **반환 `int`** (EOF 구분) |
| `getchar` | `int getchar(void)` | `fgetc(stdin)` |
| `gets` | — | **C11에서 제거**. 사용 금지 |

- `fgetc` 반환을 `char`로 받으면 EOF(-1) 판별 실패 → `int` 필수
- `fgets`는 개행 문자 포함 → `s[strcspn(s, "\n")] = '\0';`로 제거

### `EOF`와 `fgetc` 반환 타입

`EOF` — `<stdio.h>` 정의 매크로. 값 **`-1`**. 파일 끝·오류를 나타내는 **문자 아닌 신호**

`fgetc`가 `int`를 반환하는 이유 — 0~255 바이트 값 **전부**와 `EOF`(-1)를 **구분 가능한 범위** 필요. `char`(-128~127)로는 표현 충돌 발생

```c
#include <stdio.h>
#include <limits.h>

int main(void) {
    printf("EOF     = %d\n", EOF);
    printf("CHAR_MIN= %d, CHAR_MAX = %d\n", CHAR_MIN, CHAR_MAX);

    unsigned char data[] = {'A', 0xFF, 'B'};      /* 0xFF = 중간에 낀 바이트 */
    FILE *fp = fopen("bin.dat", "wb");
    fwrite(data, 1, sizeof(data), fp);
    fclose(fp);

    int count = 0;
    fp = fopen("bin.dat", "rb");
    char c;                                       /* ← 잘못된 타입 */
    while ((c = fgetc(fp)) != EOF) count++;
    fclose(fp);
    printf("char로 받은 경우: %d개 (실제 3개)\n", count);

    count = 0;
    fp = fopen("bin.dat", "rb");
    int ci;                                       /* ← 올바른 타입 */
    while ((ci = fgetc(fp)) != EOF) count++;
    fclose(fp);
    printf("int으로 받은 경우: %d개\n", count);

    remove("bin.dat");
    return 0;
}
```

```bash
cc -Wall -Wextra eofdemo.c -o eofdemo && ./eofdemo
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o eofdemo` — 출력 파일명을 `eofdemo`로 지정. 미지정 시 `a.out`
- `&& ./eofdemo` — 컴파일 성공 시에만 실행

```
EOF     = -1
CHAR_MIN= -128, CHAR_MAX = 127
char로 받은 경우: 1개 (실제 3개)
int으로 받은 경우: 3개
```

- `char`(부호 있음)로 받으면 바이트 `0xFF` → **`-1`로 변환** → `EOF`와 동일 → 2번째 바이트에서 조기 중단
- 결과 1개 — `'A'`만 읽고 데이터 중간에서 종료. **데이터 유실이 조용히 발생**
- `int`으로 받으면 `0xFF`가 `255`로 보존 → `EOF`(-1)와 구분 → 3개 정상
- **`-Wall -Wextra`로도 미검출** — 컴파일러 경고 부재. 타입 선택으로만 방어

| 타입 | 0xFF 바이트 | EOF 구분 |
|---|---|---|
| `char` (부호 있음) | `-1` | **불가** — 충돌 |
| `int` | `255` | 가능 |

- 텍스트 전용 처리에서는 우연히 동작 → 이진 데이터·UTF-8 한글에서 발현. 한글은 `0xEC` 등 상위 바이트 사용
- `EOF` 확인 후 원인 구분 — `feof(fp)`(정상 끝) vs `ferror(fp)`(오류). `fgetc` 반환만으로는 판별 불가

## 표준 스트림

| 이름 | fd | 용도 | 버퍼링 |
|---|---|---|---|
| `stdin` | 0 | 표준 입력 | 라인 |
| `stdout` | 1 | 표준 출력 | 터미널=라인, 파이프=전체 |
| `stderr` | 2 | 오류 출력 | **무버퍼** |

```c
fprintf(stderr, "stderr 출력\n");
```

- 오류·진단 메시지는 `stderr` — 리다이렉션(`> file`) 시 분리 가능, 즉시 출력
- 프롬프트처럼 개행 없는 `stdout` 출력 → `fflush(stdout)` 필요
- 버퍼링 모드 결정 규칙·플러시 시점 상세 → [06 표준 입출력 버퍼링](06-stdio-buffering.md)

## 파일 입출력

### 기본 함수

| 함수 | 시그니처 | 용도 |
|---|---|---|
| `fopen` | `FILE *fopen(const char *path, const char *mode)` | 열기 |
| `fclose` | `int fclose(FILE *fp)` | 닫기 |
| `fgets` / `fputs` | | 줄 단위 입출력 |
| `fread` / `fwrite` | `size_t fread(void *p, size_t sz, size_t n, FILE *fp)` | 이진 입출력 |
| `fseek` | `int fseek(FILE *fp, long off, int whence)` | 위치 이동 |
| `ftell` | `long ftell(FILE *fp)` | 현재 위치 |
| `rewind` | `void rewind(FILE *fp)` | 처음으로 |
| `feof` / `ferror` | | 상태 확인 |
| `fflush` | `int fflush(FILE *fp)` | 버퍼 강제 출력. 상세 → [06 버퍼링](06-stdio-buffering.md) |
| `remove` | `int remove(const char *path)` | 파일 삭제 |
| `rename` | `int rename(const char *old, const char *new)` | 이름 변경 |

### 모드 문자열

| 모드 | 의미 | 파일 없을 때 | 기존 내용 |
|---|---|---|---|
| `"r"` | 읽기 | 실패(`NULL`) | 유지 |
| `"w"` | 쓰기 | 생성 | **삭제** |
| `"a"` | 추가 | 생성 | 유지 (끝에 추가) |
| `"r+"` | 읽기·쓰기 | 실패 | 유지 |
| `"w+"` | 읽기·쓰기 | 생성 | **삭제** |
| `"a+"` | 읽기·추가 | 생성 | 유지 |
| `"rb"` 등 | 이진 모드 | | Windows에서만 의미 차이 |

### 전체 예제

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *fp = fopen("test.txt", "w");
    if (fp == NULL) { perror("fopen"); return 1; }   // 반환값 검사 필수
    fprintf(fp, "첫째 줄\n둘째 줄\n숫자 42\n");
    fclose(fp);

    fp = fopen("test.txt", "r");
    if (fp == NULL) { perror("fopen"); return 1; }
    char line[256];
    int n = 1;
    while (fgets(line, sizeof(line), fp) != NULL)
        printf("%d: %s", n++, line);                 // line에 개행 포함
    fclose(fp);

    fp = fopen("test.txt", "a");
    fprintf(fp, "추가된 줄\n");
    fclose(fp);

    fp = fopen("test.txt", "r");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    printf("파일 크기: %ld 바이트\n", size);
    rewind(fp);
    int c = fgetc(fp);
    printf("첫 바이트: 0x%02x\n", c);
    fclose(fp);

    fp = fopen("없는파일.txt", "r");
    if (fp == NULL) perror("없는파일.txt");           // 오류 메시지 출력

    remove("test.txt");
    printf("삭제 후 재열기: %s\n", fopen("test.txt","r") == NULL ? "NULL (정상)" : "존재");
    return 0;
}
```

```bash
cc -Wall -Wextra file.c -o file && ./file
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-o file` — 출력 파일명을 `file`로 지정. 미지정 시 `a.out`
- `&& ./file` — 컴파일 성공 시에만 실행

```
없는파일.txt: No such file or directory
1: 첫째 줄
2: 둘째 줄
3: 숫자 42
파일 크기: 46 바이트
첫 바이트: 0xec
삭제 후 재열기: NULL (정상)
```

- `stderr`(`perror`)가 무버퍼 → `stdout`보다 먼저 출력됨. 출력 순서 역전은 정상 동작
- 파일 크기 46바이트 — 한글이 UTF-8 3바이트씩 차지
- 첫 바이트 `0xec` — '첫'의 UTF-8 첫 바이트. `fgetc`는 **바이트 단위**, 문자 단위 아님

### `fseek` 기준점

| 상수 | 기준 |
|---|---|
| `SEEK_SET` | 파일 시작 |
| `SEEK_CUR` | 현재 위치 |
| `SEEK_END` | 파일 끝 |

파일 크기 구하기 관용 패턴 — `fseek(fp, 0, SEEK_END)` → `ftell(fp)` → `rewind(fp)`

### 이진 입출력

```c
typedef struct { int id; double score; } Record;

Record r = {1, 95.5};
FILE *fp = fopen("data.bin", "wb");
fwrite(&r, sizeof(Record), 1, fp);      // 구조체 1개 기록
fclose(fp);

Record r2;
fp = fopen("data.bin", "rb");
if (fread(&r2, sizeof(Record), 1, fp) == 1) {
    printf("%d %.1f\n", r2.id, r2.score);
}
fclose(fp);
```

```
1 95.5
```

- 반환값 = 실제 읽고 쓴 **항목 수**(바이트 아님). 검사 필수
- 주의 — 구조체 이진 저장은 패딩·엔디안·컴파일러에 종속. 다른 환경 간 교환 부적합

## 오류 처리 — `perror`

```c
FILE *fp = fopen("없는파일.txt", "r");
if (fp == NULL) perror("없는파일.txt");
```

```
없는파일.txt: No such file or directory
```

- `errno` 값에 해당하는 시스템 메시지를 `stderr`로 출력
- `strerror(errno)` — 메시지를 문자열로 획득 (`<string.h>`)

## Java와의 차이

| 항목 | Java | C |
|---|---|---|
| 출력 | `System.out.println` | `printf` (서식 필수) |
| 타입 안전 | 컴파일러 검사 | **서식·인자 불일치 미검출** (`-Wall`이 일부 경고) |
| 파일 열기 | `new FileReader(...)` | `fopen` — `NULL` 반환 검사 |
| 자원 해제 | try-with-resources | `fclose` 수동 |
| 예외 | `IOException` | 반환값·`errno` 검사 |
| 문자 인코딩 | `String`이 유니코드 | **바이트 단위**. UTF-8은 다중 바이트 |
| 버퍼 플러시 | 대부분 자동 | `fflush` 명시 필요 상황 존재 |

## 함정 · 주의점

- `printf` 서식과 인자 타입 불일치 → 정의되지 않은 동작. `-Wall`이 대부분 경고
  ```c
  printf("%d\n", 3.14);      // ← 경고. 쓰레기 값 출력
  printf("%s\n", 42);        // ← 크래시 가능
  ```
- `%zu` 대신 `%d`로 `sizeof` 출력 → 64비트에서 오동작
- `scanf("%s", buf)` — 폭 미지정 → 버퍼 오버플로. `%31s` 형태 필수
- `scanf` 인자에 `&` 누락 → 크래시
- `gets` 사용 → C11에서 제거. `fgets` 사용
- `fgetc` 반환을 `char`로 받음 → EOF 판별 실패
- `fopen` 반환값 미검사 → `NULL` 역참조 크래시
- `fclose` 누락 → 버퍼 미플러시로 내용 유실, fd 누수
- `"w"` 모드로 기존 파일 열기 → **내용 즉시 삭제**. 의도 확인 필수
- `fgets` 결과에 개행 포함 → `strcmp` 비교 실패
- `feof`로 루프 조건 구성 → 마지막 줄 중복 처리. `fgets`/`fread` 반환값으로 판단
  ```c
  while (!feof(fp)) { fgets(...); ... }      // ← 잘못된 패턴
  while (fgets(line, sizeof(line), fp)) {}   // ← 올바른 패턴
  ```
- 개행 없는 `printf` 후 즉시 종료·`fork` → 버퍼 미플러시. `fflush(stdout)` 필요

## 검증

- [ ] 각 서식 지정자 출력 확인
- [ ] `snprintf` 잘림 감지 동작
- [ ] 파일 생성·읽기·추가·삭제 전 과정
- [ ] `fopen` 실패 시 `perror` 메시지 확인
- [ ] `fseek`·`ftell`로 파일 크기 확인
- [ ] `-Wall`로 서식 불일치 경고 확인

## 다음 문서

- [[C/docs/07-stdlib/02-string|`<string.h>` 문자열 처리]]

## 관련 문서

- [[C/docs/07-stdlib/README|라이브러리 시리즈 개요]] — 빈출 함수 30선과 통합 예제
- [[C/docs/README|학습 문서 인덱스]] — 카테고리별 전체 문서 목록
- [[C/docs/07-stdlib/05-posix|POSIX 시스템 호출]] — 저수준 I/O와 프로세스 시스템 콜
- [[C/docs/07-stdlib/06-stdio-buffering|표준 입출력 버퍼링과 fflush]] — 출력이 즉시 나가지 않는 이유와 플러시 제어
- [[C/docs/08-syntax/character-literal|문자 리터럴과 문자열 리터럴]] — `'\n'`이 `int` 상수인 이유와 `EOF` 정수 비교
