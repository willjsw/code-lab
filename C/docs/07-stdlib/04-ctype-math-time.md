---
tags:
  - lang/c
  - c/stdlib
  - ctype
  - math
  - time
  - limits
  - status/verified
aliases:
  - ctype.h
  - math.h
  - time.h
created: 2026-08-14
updated: 2026-08-14
---

# `<ctype.h>` · `<math.h>` · `<time.h>` · `<limits.h>` 외

> 문자 분류, 수학 함수, 시간 처리, 타입 한계값, 불리언, 단언

## `<ctype.h>` — 문자 분류 · 변환

```c
#include <ctype.h>
```

| 함수 | 참인 조건 |
|---|---|
| `isalpha(c)` | 알파벳 |
| `isdigit(c)` | 숫자 `0`~`9` |
| `isalnum(c)` | 알파벳 또는 숫자 |
| `isspace(c)` | 공백·탭·개행·수직탭·폼피드·캐리지리턴 |
| `isupper(c)` / `islower(c)` | 대문자 / 소문자 |
| `ispunct(c)` | 구두점(출력 가능, 영숫자·공백 제외) |
| `isprint(c)` | 출력 가능 (공백 포함) |
| `isgraph(c)` | 출력 가능 (공백 제외) |
| `iscntrl(c)` | 제어 문자 |
| `isxdigit(c)` | 16진 숫자 |
| `toupper(c)` / `tolower(c)` | 대/소문자 변환 |

```c
printf("isalpha('a')=%d isdigit('5')=%d isalnum('_')=%d isspace(' ')=%d\n",
       !!isalpha('a'), !!isdigit('5'), !!isalnum('_'), !!isspace(' '));
printf("isupper('A')=%d islower('A')=%d ispunct('!')=%d\n",
       !!isupper('A'), !!islower('A'), !!ispunct('!'));
printf("toupper('a')='%c' tolower('Z')='%c'\n", toupper('a'), tolower('Z'));
```

```
isalpha('a')=1 isdigit('5')=1 isalnum('_')=0 isspace(' ')=1
isupper('A')=1 islower('A')=0 ispunct('!')=1
toupper('a')='A' tolower('Z')='z'
```

- 반환값은 "참이면 **0이 아닌 값**" — 반드시 1이 아님. `== 1` 비교 금지
- 위 예제의 `!!` — 0/1로 정규화 (출력 편의)
- 인자·반환 타입이 `int` — `char`를 직접 전달 시 음수 값 문제 발생 가능
  ```c
  char c = str[i];
  isalpha((unsigned char)c);    // ← 캐스팅 권장
  ```
  UTF-8 한글 등은 `char`가 음수가 되어 정의되지 않은 동작 유발
- 변환 함수는 조건 불일치 시 원본 반환 — `toupper('1')` → `'1'`

활용 — 문자열 대문자 변환

```c
for (size_t i = 0; s[i]; i++)
    s[i] = (char)toupper((unsigned char)s[i]);
```

## `<math.h>` — 수학 함수

```c
#include <math.h>
```

**Linux에서 `-lm` 링크 필수**. macOS는 불필요

```bash
cc main.c -o main -lm
```

- `-o main` — 출력 파일명을 `main`로 지정. 미지정 시 `a.out`
- `-lm` — 수학 라이브러리 링크. **Linux 필수, macOS 불필요**

### 주요 함수

| 분류 | 함수 | 용도 |
|---|---|---|
| 거듭제곱·근 | `pow(x,y)` `sqrt(x)` `cbrt(x)` `hypot(x,y)` | 제곱·제곱근·세제곱근·빗변 |
| 지수·로그 | `exp(x)` `log(x)` `log10(x)` `log2(x)` | `log`는 자연로그 |
| 반올림 | `ceil(x)` `floor(x)` `round(x)` `trunc(x)` | 올림·내림·반올림·버림 |
| 절댓값·나머지 | `fabs(x)` `fmod(x,y)` | 실수 전용 |
| 삼각 | `sin` `cos` `tan` `asin` `acos` `atan` `atan2(y,x)` | 라디안 단위 |
| 쌍곡선 | `sinh` `cosh` `tanh` | |
| 판정 | `isnan(x)` `isinf(x)` `isfinite(x)` | 특수값 검사 |
| 최대·최소 | `fmax(x,y)` `fmin(x,y)` | |

```c
printf("sqrt(2)=%.4f pow(2,10)=%.0f fabs(-3.5)=%.1f\n", sqrt(2.0), pow(2,10), fabs(-3.5));
printf("ceil(2.1)=%.0f floor(2.9)=%.0f round(2.5)=%.0f trunc(-2.7)=%.0f\n",
       ceil(2.1), floor(2.9), round(2.5), trunc(-2.7));
printf("fmod(7,3)=%.1f log(e)=%.1f log10(1000)=%.1f exp(1)=%.4f\n",
       fmod(7.0,3.0), log(M_E), log10(1000.0), exp(1.0));
printf("sin(0)=%.1f cos(0)=%.1f M_PI=%.6f\n", sin(0.0), cos(0.0), M_PI);
```

```
sqrt(2)=1.4142 pow(2,10)=1024 fabs(-3.5)=3.5
ceil(2.1)=3 floor(2.9)=2 round(2.5)=3 trunc(-2.7)=-2
fmod(7,3)=1.0 log(e)=1.0 log10(1000)=3.0 exp(1)=2.7183
sin(0)=0.0 cos(0)=1.0 M_PI=3.141593
```

### 상수

| 상수 | 값 |
|---|---|
| `M_PI` | 원주율 |
| `M_E` | 자연상수 e |
| `INFINITY` | 무한대 |
| `NAN` | Not a Number |

- `M_PI` 등은 POSIX 확장. 엄격한 표준 모드(`-std=c11`)에서 미정의 가능 → `-std=gnu11` 또는 직접 정의

### 실수 비교 주의

```c
if (a == b)                          // ← 부동소수점 오차로 실패 가능
if (fabs(a - b) < 1e-9)              // ← 허용 오차 기반 비교
```

- `0.1 + 0.2 != 0.3` — 이진 부동소수점의 근본 한계
- `NAN`은 자기 자신과도 다름 → `x != x`가 NaN 판정. `isnan(x)` 사용

### 정수 나눗셈 함정

```c
int a = 7, b = 2;
double r1 = a / b;              // 3.0  ← 정수 나눗셈 후 변환
double r2 = (double)a / b;      // 3.5  ← 캐스팅 후 나눗셈
```

## `<time.h>` — 시간 처리

```c
#include <time.h>
```

| 함수 · 타입 | 용도 |
|---|---|
| `time_t` | 에포크(1970-01-01 UTC) 이후 초 |
| `time(NULL)` | 현재 시각 획득 |
| `struct tm` | 년·월·일·시·분·초 분해 구조체 |
| `localtime(&t)` | `time_t` → 지역 시간 `struct tm *` |
| `gmtime(&t)` | `time_t` → UTC `struct tm *` |
| `mktime(&tm)` | `struct tm` → `time_t` |
| `strftime(buf, n, fmt, tm)` | 서식 문자열 생성 |
| `difftime(t2, t1)` | 초 단위 차이 (`double`) |
| `clock()` | CPU 사용 시간 (`CLOCKS_PER_SEC` 단위) |

```c
time_t t = 1700000000;
struct tm *lt = localtime(&t);
char buf[64];
strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
printf("고정 timestamp 1700000000 → %s\n", buf);
printf("CLOCKS_PER_SEC=%ld\n", (long)CLOCKS_PER_SEC);
```

```
고정 timestamp 1700000000 → 2023-11-15 07:13:20
CLOCKS_PER_SEC=1000000
```

- 위 출력은 **KST(UTC+9) 기준**. `localtime`은 시스템 시간대에 의존 → 환경마다 상이

### `struct tm` 필드 (주의)

| 필드 | 범위 | 함정 |
|---|---|---|
| `tm_year` | 1900년 기준 | **실제 연도 - 1900** |
| `tm_mon` | 0~11 | **0이 1월** |
| `tm_mday` | 1~31 | 정상 |
| `tm_hour` `tm_min` `tm_sec` | 0~23, 0~59, 0~60 | 60은 윤초 |
| `tm_wday` | 0~6 | 0이 일요일 |
| `tm_yday` | 0~365 | 연중 일자 |

```c
struct tm tm = {0};
tm.tm_year = 2026 - 1900;      // ← 1900 차감 필수
tm.tm_mon  = 8 - 1;            // ← 8월이면 7
tm.tm_mday = 14;
time_t t = mktime(&tm);
```

### `strftime` 서식

| 서식 | 의미 |
|---|---|
| `%Y` `%m` `%d` | 연(4자리)·월·일 |
| `%H` `%M` `%S` | 시(24)·분·초 |
| `%A` `%B` | 요일명·월명 |
| `%j` | 연중 일자 |
| `%Z` `%z` | 시간대 이름·오프셋 |
| `%F` | `%Y-%m-%d` 축약 |
| `%T` | `%H:%M:%S` 축약 |

### 경과 시간 측정

```c
clock_t start = clock();
// ... 작업 ...
double sec = (double)(clock() - start) / CLOCKS_PER_SEC;
```

- `clock()` — **CPU 시간**. 대기 시간 미포함
- 실제 경과 시간은 `time()` 차이 또는 `clock_gettime(CLOCK_MONOTONIC, ...)` (POSIX)
```
2천만 회 루프: 0.029초 (CPU 시간)
```

### 스레드 안전성

- `localtime`·`gmtime`은 **정적 버퍼 반환** → 재호출 시 이전 결과 덮어씀
- 스레드 환경 — `localtime_r(&t, &tm)`·`gmtime_r` 사용 (POSIX)

## `<limits.h>` · `<float.h>` — 타입 한계값

```c
#include <limits.h>
#include <float.h>
```

```c
printf("INT_MAX=%d INT_MIN=%d\n", INT_MAX, INT_MIN);
printf("LONG_MAX=%ld CHAR_BIT=%d\n", LONG_MAX, CHAR_BIT);
printf("sizeof: char=%zu int=%zu long=%zu double=%zu ptr=%zu\n",
       sizeof(char), sizeof(int), sizeof(long), sizeof(double), sizeof(void*));
printf("DBL_MAX=%g DBL_EPSILON=%g\n", DBL_MAX, DBL_EPSILON);
```

```
INT_MAX=2147483647 INT_MIN=-2147483648
LONG_MAX=9223372036854775807 CHAR_BIT=8
sizeof: char=1 int=4 long=8 double=8 ptr=8
DBL_MAX=1.79769e+308 DBL_EPSILON=2.22045e-16
```

| 상수 | 의미 |
|---|---|
| `CHAR_BIT` | 바이트당 비트 수 (보통 8) |
| `SCHAR_MIN` `SCHAR_MAX` `UCHAR_MAX` | `char` 계열 한계 |
| `SHRT_MIN` `SHRT_MAX` `USHRT_MAX` | `short` |
| `INT_MIN` `INT_MAX` `UINT_MAX` | `int` |
| `LONG_MIN` `LONG_MAX` `ULONG_MAX` | `long` |
| `LLONG_MIN` `LLONG_MAX` | `long long` |
| `DBL_MAX` `DBL_MIN` `DBL_EPSILON` | `double` |
| `FLT_MAX` `FLT_EPSILON` | `float` |

**타입 크기는 플랫폼 종속** — 위 값은 macOS arm64 기준

| 타입 | 일반적 크기 | 비고 |
|---|---|---|
| `char` | 1 | 항상 1 (표준 보장) |
| `short` | 2 | |
| `int` | 4 | |
| `long` | **8 (Unix 64bit) / 4 (Windows)** | 이식성 주의 |
| `long long` | 8 | |
| `float` `double` | 4, 8 | |
| 포인터 | 8 (64비트) | |

- 고정 크기 필요 시 `<stdint.h>`의 `int32_t`·`uint64_t` 등 사용

## `<stdbool.h>` — 불리언

```c
#include <stdbool.h>

bool flag = true;
printf("bool true=%d false=%d\n", flag, false);
```

```
bool true=1 false=0
```

- C99 도입. 실체는 `_Bool` 타입의 매크로 별칭
- 미사용 시 관용 표현 — `int` 사용, 0=거짓, 0 이외=참
- C에서 **0만 거짓**. `NULL`·`'\0'`·`0.0` 전부 거짓

## `<assert.h>` — 단언

```c
#include <assert.h>

assert(1 + 1 == 2);
```

- 조건 거짓 시 메시지 출력 후 `abort()` 호출
- `-DNDEBUG` 컴파일 시 **전부 제거** → 배포 빌드에서 무효화
- **주의** — `assert` 안에 부수 효과 있는 코드 배치 금지
  ```c
  assert(process(x) == 0);     // ← NDEBUG 빌드에서 process 미호출!
  int r = process(x);
  assert(r == 0);              // ← 올바른 형태
  ```
- 용도 — 개발 중 "절대 일어나면 안 되는" 조건 검증. 사용자 입력 검증에는 부적합

## `<stdint.h>` — 고정 크기 정수

```c
#include <stdint.h>
```

| 타입 | 크기 |
|---|---|
| `int8_t` `uint8_t` | 1바이트 |
| `int16_t` `uint16_t` | 2바이트 |
| `int32_t` `uint32_t` | 4바이트 |
| `int64_t` `uint64_t` | 8바이트 |
| `intptr_t` `uintptr_t` | 포인터 크기 |
| `size_t` | 크기·인덱스 (부호 없음, `<stddef.h>`) |
| `ssize_t` | 크기 + 오류(-1) (POSIX) |
| `ptrdiff_t` | 포인터 차이 |

- 출력 서식 — `<inttypes.h>`의 `PRId32`·`PRIu64` 매크로
  ```c
  #include <inttypes.h>
  printf("%" PRId64 "\n", (int64_t)123);
  ```
- 파일 형식·네트워크 프로토콜처럼 **정확한 크기가 중요한 경우** 사용

## `<errno.h>` — 오류 코드

```c
#include <errno.h>
```

| 항목 | 설명 |
|---|---|
| `errno` | 마지막 오류 번호 (스레드별 값) |
| `ENOENT` | 파일·디렉토리 부재 |
| `EACCES` | 권한 거부 |
| `ENOMEM` | 메모리 부족 |
| `EINVAL` | 잘못된 인자 |
| `ERANGE` | 범위 초과 |
| `EINTR` | 시그널로 중단됨 |
| `EAGAIN` | 재시도 필요 |

사용 규칙

```c
errno = 0;                     // 호출 전 초기화
long v = strtol(s, &end, 10);
if (errno == ERANGE) { /* 범위 오류 */ }
```

- **성공 시 `errno`는 초기화되지 않음** → 함수가 실패를 반환했을 때만 검사
- 메시지 변환 — `strerror(errno)` 또는 `perror("컨텍스트")`

## 헤더 요약

| 헤더 | 주 용도 |
|---|---|
| `<stdio.h>` | 입출력 |
| `<stdlib.h>` | 메모리·변환·유틸 |
| `<string.h>` | 문자열·메모리 조작 |
| `<ctype.h>` | 문자 분류 |
| `<math.h>` | 수학 함수 (`-lm`) |
| `<time.h>` | 시간 |
| `<limits.h>` `<float.h>` | 타입 한계 |
| `<stdbool.h>` | `bool` |
| `<stdint.h>` `<inttypes.h>` | 고정 크기 정수 |
| `<assert.h>` | 단언 |
| `<errno.h>` | 오류 코드 |
| `<stddef.h>` | `size_t`·`NULL`·`offsetof` |
| `<stdarg.h>` | 가변 인자 (`va_list`) |
| `<signal.h>` | 시그널 |
| `<setjmp.h>` | 비지역 점프 (권장 부재) |

## 함정 · 주의점

- `ctype` 함수에 `char` 직접 전달 → 음수 인덱스. `(unsigned char)` 캐스팅
- `ctype` 반환값을 `== 1`로 비교 → 참은 "0 이외"의 임의 값
- Linux에서 `-lm` 누락 → `undefined reference to 'sqrt'`
- 실수 `==` 비교 → 오차로 실패. `fabs(a-b) < 엡실론`
- `tm_year`에 실제 연도 대입 → 1900 차감 누락
- `tm_mon`에 1~12 대입 → 0~11 범위. 1 차감 필요
- `localtime` 결과 포인터 보관 → 재호출 시 덮어씀. 값 복사 또는 `localtime_r`
- `assert` 안에 부수 효과 → `NDEBUG` 빌드에서 미실행
- `long`을 8바이트로 가정 → Windows에서 4바이트. `int64_t` 사용
- `errno`를 성공 시에도 검사 → 이전 값 오판. 호출 전 `errno = 0`
- 정수 나눗셈 후 실수 대입 → 소수부 소실. 캐스팅 위치 주의

## 검증

- [ ] `ctype` 각 함수 판정 결과 확인
- [ ] `math` 함수 결과 및 `-lm` 필요 여부 확인
- [ ] `strftime` 출력 확인
- [ ] `sizeof`로 타입 크기 확인
- [ ] `assert` 실패 시 동작 및 `-DNDEBUG` 무효화 확인

## 다음 문서

- [[C/docs/07-stdlib/05-posix|POSIX 시스템 호출]]

## 관련 문서

- [[C/docs/07-stdlib/03-stdlib|`<stdlib.h>` 메모리 · 변환]] — 동적 메모리와 변환·정렬
- [[C/docs/07-stdlib/README|라이브러리 시리즈 개요]] — 빈출 함수 30선과 통합 예제
