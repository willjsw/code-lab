---
tags:
  - topic/os
  - ostep/virtualization
  - lang/c
  - project/ostep-04
  - malloc
  - memory-leak
  - asan
  - use-after-free
  - status/verified
aliases:
  - OSTEP 실습 4
  - 메모리 오류 실습
created: 2026-08-19
updated: 2026-08-19
---

# 실습 4. 메모리 API와 오류 검출 (ch14)

> 원서가 열거한 메모리 오류 전 유형을 직접 재현하고, **대부분이 "정상처럼" 동작한다는 사실**을 확인한 뒤 AddressSanitizer 로 지점을 지목

- 이론 — [[OS/ostep/docs/01-virtualization/14-memory-api|14. 메모리 API]]
- 원서 PDF — [14-Memory-API.pdf](../../pdfs/01-virtualization/14-Memory-API.pdf) (14.4 Common Errors + 권말 Homework Code)

> [!tip] TIP: 컴파일되었거나 실행되었다 ≠ 올바르다 (원서 TIP)
> 프로그램이 컴파일되었다는 것, 심지어 여러 번 올바르게 실행되었다는 것이 **프로그램이 올바르다는 뜻은 아님**. 학생의 흔한 반응은 "But it worked before!" 라며 컴파일러·OS·하드웨어·교수를 탓하는 것. 그러나 문제는 보통 **자기 코드**에 있음

## 파일 구성

| 파일 | 오류 유형 | 원서 대응 |
|---|---|---|
| `01-null.c` | NULL 역참조 | 숙제 1 |
| `02-unallocated.c` | 할당을 잊음 | 14.4 Forgetting To Allocate Memory |
| `03-too-small.c` | 할당 크기 부족 (버퍼 오버플로) | 14.4 Not Allocating Enough Memory |
| `04-uninitialized.c` | 초기화를 잊음 (미초기화 읽기) | 14.4 Forgetting To Initialize |
| `05-leak.c` | `free` 를 잊음 (메모리 누수) | 14.4 Forgetting To Free / 숙제 4 |
| `06-overflow.c` | 배열 경계 밖 접근 | 숙제 5 |
| `07-use-after-free.c` | 해제 후 사용 (댕글링 포인터) | 14.4 Freeing Before Done / 숙제 6 |
| `08-double-free.c` | 이중 해제 | 14.4 Freeing Memory Repeatedly |
| `09-invalid-free.c` | 잘못된 `free` | 14.4 Calling free() Incorrectly |

## macOS 에서의 도구 대체

원서는 `valgrind` 사용을 전제하나, **Apple Silicon macOS 에서 valgrind 는 사실상 사용 불가**.

| 원서 도구 | macOS 대체 | 검출 범위 |
|---|---|---|
| `valgrind --leak-check=yes` | **AddressSanitizer** (`-fsanitize=address`) | 오버플로 · use-after-free · double free · bad free · SEGV |
| (누수 검출) | **`leaks` 명령** (macOS 기본 제공) | 메모리 누수 |
| `gdb` | **`lldb`** | 디버깅 |

> [!note] ASIDE: macOS arm64 에서 LeakSanitizer 미지원
> ASan 의 누수 검출 기능은 이 플랫폼에서 동작하지 않음. 실측 —
> ```text
> ==31323==AddressSanitizer: detect_leaks is not supported on this platform.
> ```
> 누수는 `leaks` 명령으로 별도 확인

## 빌드

```bash
make
```

- 옵션 없음. 각 소스를 **일반 빌드**와 **ASan 빌드** 두 가지로 컴파일

```text
gcc -Wall -Wextra -g -o 01-null 01-null.c
gcc -Wall -Wextra -g -o 02-unallocated 02-unallocated.c
02-unallocated.c:9:20: warning: variable 'dst' is uninitialized when used here [-Wuninitialized]
    9 |     printf("%s\n", dst);
      |                    ^~~
02-unallocated.c:7:14: note: initialize the variable 'dst' to silence this warning
    7 |     char *dst;              // oops! 미할당 — 쓰레기 주소
      |              ^
      |               = NULL
1 warning generated.
...
gcc -Wall -Wextra -g -fsanitize=address -fno-omit-frame-pointer -o 01-null-asan 01-null.c
```

- `-Wall` — 주요 경고 활성
- `-Wextra` — 추가 경고 활성
- `-g` — 디버그 심볼 포함. ASan·lldb 가 **행 번호를 표시하는 데 필수**
- `-fsanitize=address` — 메모리 오류 검사 코드 삽입 (ASan 빌드 전용)
- `-fno-omit-frame-pointer` — 프레임 포인터 유지. ASan 스택 트레이스 정확도 향상 (ASan 빌드 전용)
- `-o <이름>` — 출력 실행 파일명 지정

**`02-unallocated.c` 는 컴파일 시점에 `-Wall` 이 잡아냄** — 미초기화 포인터 사용은 컴파일러가 경고 가능한 소수 사례

## 1단계 — 일반 빌드 실행 결과 (핵심)

```bash
./01-null; echo "종료 코드 $?"
```

- 각 프로그램을 옵션 없이 실행하고 `$?` 로 종료 코드 확인
- `echo "종료 코드 $?"` — 직전 명령의 종료 코드 출력. 파이프를 쓰면 파이프 마지막 명령의 코드가 잡히므로 **파이프 없이** 실행할 것

| 프로그램 | 출력 | 종료 코드 | 판정 |
|---|---|---|---|
| `01-null` | (없음) | **139** = 128+11 (SIGSEGV) | **크래시** |
| `02-unallocated` | `hello` | 0 | **정상처럼 동작** |
| `03-too-small` | `hello` | 0 | **정상처럼 동작** |
| `04-uninitialized` | `초기화 전 값: 0` / `calloc 값 : 0` | 0 | **정상처럼 동작** (운 좋게 0) |
| `05-leak` | 할당 3회 주소 출력 | 0 | **정상처럼 동작** (누수는 조용함) |
| `06-overflow` | `data[100] = 0` | 0 | **정상처럼 동작** |
| `07-use-after-free` | `data[0] = 7` | 0 | **정상처럼 동작** — 단, **넣은 값 42 가 아님** |
| `08-double-free` | (없음) | **133** = 128+5 (SIGTRAP) | **크래시** |
| `09-invalid-free` | (없음) | **133** = 128+5 (SIGTRAP) | **크래시** |

**결정적 관찰**

- **9개 중 6개가 종료 코드 0 으로 "성공"** — 원서 TIP "It Compiled Or It Ran ≠ It Is Correct" 의 실증
- `07-use-after-free` 는 특히 위험 — `data[0] = 42` 를 넣었으나 해제 후 읽으면 **`7`** 이 나옴. 해제된 영역이 할당자 내부 자료 구조로 재사용된 결과. **크래시하지 않고 조용히 틀린 값**을 반환
- `08`·`09` 는 macOS `malloc` 이 자체 검증으로 즉시 중단시킴. 다만 이 환경에서 **stderr 메시지는 출력되지 않음** (`os_log` 경로 사용 추정 — 확인 필요)

## 2단계 — ASan 빌드 실행 결과

```bash
./03-too-small-asan
```

- 옵션 없음

```text
=================================================================
==31256==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x6020000000f5 at pc 0x00010333aa20 bp 0x00016d49a760 sp 0x00016d499f10
WRITE of size 6 at 0x6020000000f5 thread T0
    #0 0x00010333aa1c in strcpy+0x458 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x3aa1c)
    #1 0x00010296481c in main 03-too-small.c:10
    #2 0x00018124fdfc in start+0x1b4c (dyld:arm64e+0x1fdfc)

0x6020000000f5 is located 0 bytes after 5-byte region [0x6020000000f0,0x6020000000f5)
allocated by thread T0 here:
    #0 0x000103341164 in malloc+0x78 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x41164)
    #1 0x00010296480c in main 03-too-small.c:9

SUMMARY: AddressSanitizer: heap-buffer-overflow 03-too-small.c:10 in main
```

- **`WRITE of size 6`** — `"hello"` 5글자 + 종료 문자 `'\0'` = 6바이트를 기록하려 함
- **`5-byte region` 바로 뒤 0바이트 지점**을 침범 → `malloc(strlen(src))` 가 정확히 1바이트 부족했음을 지목
- 할당 지점(`:9`)과 오류 지점(`:10`)을 **모두** 보여줌

```bash
./06-overflow-asan
```

```text
==31260==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x6140000001d0 at pc 0x0001049a885c bp 0x00016b456740 sp 0x00016b456738
WRITE of size 4 at 0x6140000001d0 thread T0
    #0 0x0001049a8858 in main 06-overflow.c:7

0x6140000001d0 is located 0 bytes after 400-byte region [0x614000000040,0x6140000001d0)
allocated by thread T0 here:
    #1 0x0001049a8804 in main 06-overflow.c:6

SUMMARY: AddressSanitizer: heap-buffer-overflow 06-overflow.c:7 in main
```

- `400-byte region` = `100 * sizeof(int)`. 그 **바로 뒤**를 4바이트 기록 → `data[100]` 이 경계 밖임을 정확히 지목

```bash
./07-use-after-free-asan
```

```text
==31264==ERROR: AddressSanitizer: heap-use-after-free on address 0x614000000040 at pc 0x0001025c88bc bp 0x00016d836730 sp 0x00016d836728
READ of size 4 at 0x614000000040 thread T0
    #0 0x0001025c88b8 in main 07-use-after-free.c:9

0x614000000040 is located 0 bytes inside of 400-byte region [0x614000000040,0x6140000001d0)
freed by thread T0 here:
    #1 0x0001025c886c in main 07-use-after-free.c:8

previously allocated by thread T0 here:
    #1 0x0001025c8804 in main 07-use-after-free.c:6

SUMMARY: AddressSanitizer: heap-use-after-free 07-use-after-free.c:9 in main
```

- **할당(`:6`) → 해제(`:8`) → 사용(`:9`)** 세 지점을 모두 지목. 일반 빌드에서는 조용히 `7` 을 반환했던 오류

```bash
./08-double-free-asan
```

```text
==31268==ERROR: AddressSanitizer: attempting double-free on 0x6020000000f0 in thread T0:
    #1 0x0001041f4874 in main 08-double-free.c:9

freed by thread T0 here:
    #1 0x0001041f486c in main 08-double-free.c:8

previously allocated by thread T0 here:
    #1 0x0001041f4804 in main 08-double-free.c:6

SUMMARY: AddressSanitizer: double-free 08-double-free.c:9 in main
==31268==ABORTING
```

```bash
./09-invalid-free-asan
```

```text
==31275==ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed: 0x604000000414 in thread T0
    #1 0x000104e0480c in main 09-invalid-free.c:7

0x604000000414 is located 4 bytes inside of 40-byte region [0x604000000410,0x604000000438)
allocated by thread T0 here:
    #1 0x000104e047fc in main 09-invalid-free.c:6

SUMMARY: AddressSanitizer: bad-free 09-invalid-free.c:7 in main
==31275==ABORTING
```

- `4 bytes inside of 40-byte region` — `free(x + 1)` 이 시작 주소가 아님을 정확히 표현 (`int` 1칸 = 4바이트)

```bash
./01-null-asan
```

```text
AddressSanitizer:DEADLYSIGNAL
=================================================================
==31316==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x00010056c850 bp 0x00016f8927b0 sp 0x00016f892780 T0)
==31316==The signal is caused by a READ memory access.
==31316==Hint: address points to the zero page.
    #0 0x00010056c850 in main 01-null.c:6
```

- `Hint: address points to the zero page` — NULL 역참조임을 명시

### ASan 이 못 잡는 것

```bash
./02-unallocated-asan
```

```text
hello
```

- **ASan 도 검출하지 못함.** 미초기화 포인터가 우연히 접근 가능한 주소를 가리키면 ASan 의 섀도 메모리 검사를 통과
- 이 오류는 **컴파일러 경고(`-Wuninitialized`)** 로만 잡혔음 → **도구 하나에 의존하지 말 것**

## 3단계 — 누수 검출 (`leaks`)

```bash
leaks --atExit -- ./05-leak
```

- `leaks` — macOS 기본 제공 누수 검출 명령
- `--atExit` — **프로세스 종료 시점**에 검사. 지정하지 않으면 실행 중 스냅숏 검사
- `--` — 이후 인자를 검사 대상 명령으로 취급

```text
Process:         05-leak [31388]
Process 31388: 192 nodes malloced for 46 KB
Process 31388: 2 leaks for 10240 total leaked bytes.
STACK OF 2 INSTANCES OF 'ROOT LEAK: <malloc in main>':
0   libsystem_malloc.dylib                0x18143a178 _malloc_zone_malloc_instrumented_or_legacy + 152 
      1 (5.00K) ROOT LEAK: <malloc in main 0xabec0c000> [5120]
      1 (5.00K) ROOT LEAK: <malloc in main 0xabec0d400> [5120]
```

- **`2 leaks for 10240 total leaked bytes`** — `main` 의 `malloc` 이 누수 지점으로 지목됨
- 프로그램은 3회 할당했으나 **2개만 검출** — 종료 시점에 마지막 포인터 값이 레지스터·스택에 남아 "도달 가능"으로 판단된 것으로 추정 (확인 필요). `leaks` 는 **보수적 검출기** — 누수를 놓칠 수는 있으나 없는 누수를 보고하지는 않음
- 요청 크기 4096바이트(`1024 * sizeof(int)`)가 `5120`(5KB)으로 보고되는 것은 할당자의 **크기 클래스 반올림** 결과

대조 — `free` 를 호출하는 프로그램.

```bash
leaks --atExit -- ./04-uninitialized
```

```text
Process 31396: 0 leaks for 0 total leaked bytes.
```

- 누수 0 확인 → 검출기가 올바르게 동작함을 보증하는 **대조군**

```bash
leaks --atExit -- ./05-leak >/dev/null 2>&1; echo "leaks 종료 코드: $?"
```

- `>/dev/null 2>&1` — 표준 출력·에러를 모두 폐기하고 종료 코드만 확인
- CI 등에서 누수 여부를 자동 판정할 때 사용

```text
leaks 종료 코드: 1
```

- 누수 발견 시 **1**, 없으면 0

> [!note] ASIDE: `leaks` 실행 시 나오는 경고
> ```text
> Process 31388 is not debuggable. Due to security restrictions, leaks can only show or save contents of readonly memory of restricted processes.
> ```
> SIP(System Integrity Protection) 관련 제약 안내. **검출 자체는 정상 동작**하므로 무시 가능

## 오류별 검출 수단 정리

| 오류 | 일반 실행 | 컴파일러 경고 | ASan | `leaks` |
|---|---|---|---|---|
| NULL 역참조 | **크래시(139)** | — | **검출** | — |
| 할당을 잊음 | 정상처럼 동작 | **검출** (`-Wuninitialized`) | 미검출 | — |
| 크기 부족 | 정상처럼 동작 | — | **검출** | — |
| 미초기화 읽기 | 정상처럼 동작 | — | 미검출 | — |
| 누수 | 정상처럼 동작 | — | 미지원(플랫폼) | **검출** |
| 경계 밖 접근 | 정상처럼 동작 | — | **검출** | — |
| 해제 후 사용 | **조용히 틀린 값** | — | **검출** | — |
| 이중 해제 | 크래시(133) | — | **검출** | — |
| 잘못된 `free` | 크래시(133) | — | **검출** | — |

- **어느 하나도 전부를 잡지 못함** → 컴파일러 경고 + ASan + `leaks` 를 **함께** 사용
- 미초기화 읽기는 세 도구 모두 놓침 → `calloc` 사용, 또는 선언과 동시 초기화를 습관화

## 함정

| 증상 | 원인 | 대응 |
|---|---|---|
| 오류가 있는데 프로그램이 잘 돌아감 | 힙 오류는 대부분 **즉시 증상을 내지 않음**. 할당자가 여유 공간을 두거나, 덮어쓴 값이 더 이상 쓰이지 않는 경우 | ASan 빌드로 항상 함께 테스트 |
| ASan 빌드가 원본보다 느림 | 검사 코드 삽입 + 섀도 메모리 사용. 통상 2~3배 | 개발·테스트 시에만 사용. 배포 빌드에서는 제거 |
| `ASAN_OPTIONS=detect_leaks=1` 이 안 먹음 | macOS arm64 는 LeakSanitizer 미지원 | `leaks` 명령 사용 |
| 파이프로 실행하니 종료 코드가 0 | `$?` 가 파이프 **마지막 명령**의 코드를 반영 | 파이프 없이 실행하거나 `${PIPESTATUS[0]}`(bash) 사용 |
| `free` 후 포인터가 여전히 값을 가짐 | `free` 는 **포인터 변수를 변경하지 않음** | `free(p); p = NULL;` 관용을 습관화 |
| 짧은 프로그램의 누수를 무시해도 되는가 | 종료 시 OS가 주소 공간 전체를 회수하므로 실질 문제 없음 | 그러나 **습관** 문제. 장기 실행 서버·커널에서는 치명적 |

## 확인 문제

1. `03-too-small.c` 의 `malloc(strlen(src))` 를 `malloc(strlen(src) + 1)` 로 고치고 ASan 을 다시 실행할 것. 오류가 사라지는가
2. `07-use-after-free.c` 의 일반 빌드가 `42` 가 아니라 `7` 을 출력하는 이유는 무엇인가. 해제된 블록이 할당자에게 어떻게 쓰이는지로 설명할 것
3. `04-uninitialized.c` 의 `malloc` 을 여러 번 반복 호출하며 값을 출력하면 항상 0 인가. 큰 크기로 바꾸면 어떻게 되는가
4. `05-leak.c` 에 `free(leaked);` 를 추가하고 `leaks` 를 다시 실행해 `0 leaks` 를 확인할 것
5. `02-unallocated.c` 를 ASan 도 잡을 수 있게 만들 수 있는가. `char *dst = NULL;` 로 초기화하면 어떻게 달라지는가
6. `lldb ./06-overflow` 로 디버거를 붙여 `data[100] = 0;` 직전에 중단(`breakpoint set -f 06-overflow.c -l 7`)하고 `data` 주소를 확인할 것

## 관련 문서

- [[OS/ostep/docs/01-virtualization/14-memory-api|14. 메모리 API]] — 본 실습의 이론 배경
- [[OS/ostep/docs/01-virtualization/17-free-space-management|17. 프리 공간 관리]] — 할당자가 해제된 블록을 어떻게 재사용하는지
- [[OS/ostep/projects/README|실습 프로젝트 인덱스]] — 전체 실습 커리큘럼
- [[C/docs/02-memory/heap-and-free|힙과 free]] — `free` 의 실제 동작과 댕글링 포인터
- [[C/docs/07-stdlib/03-stdlib|메모리 · 변환]] — `malloc`·`calloc`·`realloc`·`free` API 규칙
