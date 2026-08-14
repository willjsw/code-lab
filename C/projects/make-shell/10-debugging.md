---
tags:
  - lang/c
  - c/debugging
  - project/make-shell
  - shell
  - asan
  - lldb
  - memory-leak
  - status/wip
created: 2026-08-14
updated: 2026-08-14
---

# 10 · 디버깅 · 검증

> ASan으로 메모리 오류 조기 발견, lldb로 중단점 추적, 스크립트로 회귀 검증. macOS 환경 제약 포함

## 목표

- AddressSanitizer 출력 해석 — 오버플로·use-after-free·누수
- macOS에서의 누수 탐지 대안 확보
- lldb 기본 조작 및 자식 프로세스 추적
- 회귀 테스트 스크립트 구성

## 개념

- 메모리 오류의 특성 — **증상 발생 지점 ≠ 원인 지점**. 힙 손상 후 한참 뒤 무관한 위치에서 크래시
- AddressSanitizer(ASan) — 컴파일 시 검사 코드 삽입. 오류 발생 **즉시** 중단 및 스택 추적 출력
  - 비용 — 실행 속도 약 2배 저하, 메모리 사용 증가. 개발 중 상시 사용, 배포 빌드 제외
- `-fsanitize=address` — 컴파일·링크 양쪽 지정
- `-g` — 디버그 심볼. 미지정 시 ASan 출력에 파일·행 번호 부재
- `-fno-omit-frame-pointer` — 스택 추적 정확도 향상

## macOS 환경 제약 (중요)

- **LeakSanitizer 미지원** — arm64 macOS에서 `ASAN_OPTIONS=detect_leaks=1` 지정 시 아래 출력

```
==36878==AddressSanitizer: detect_leaks is not supported on this platform.
```

- 대안 — macOS 기본 제공 `leaks` 명령 사용. 아래 실행 예시에서 검증
- Linux에서는 ASan이 누수 탐지까지 기본 수행 → 환경 차이 인지 필요

## Java와의 차이

| 항목 | Java | C |
|---|---|---|
| 범위 초과 | `ArrayIndexOutOfBoundsException` 즉시 | 검출 부재 → 인접 메모리 손상 |
| 해제 후 접근 | 불가능 | 우연히 동작하거나 무작위 크래시 |
| 누수 | GC 대상 외 참조만 문제 | `free` 누락 = 즉시 누수 |
| 디버깅 도구 | 디버거·프로파일러 | 디버거 + 별도 sanitizer 필수 |
| 오류 시점 | 예외 발생 지점 = 원인 지점 | 증상과 원인 분리 빈번 |

## ASan 출력 해석

### 힙 버퍼 오버플로

재현 코드

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    char *buf = malloc(8);
    strcpy(buf, "0123456789");   // ← 8바이트 버퍼에 11바이트 기록
    printf("%s\n", buf);
    free(buf);
    return 0;
}
```

```bash
cc -Wall -Wextra -g -fsanitize=address bug.c -o bug && ./bug
```

- `-Wall` — 주요 경고 활성. 미초기화 변수·타입 불일치 등 검출
- `-Wextra` — `-Wall` 미포함 추가 경고 활성
- `-g` — 디버그 심볼 포함. lldb 추적·ASan 행 번호 표시에 필요
- `-fsanitize=address` — AddressSanitizer 활성. 힙 오버플로·use-after-free 즉시 검출
- `-o bug` — 출력 파일명을 `bug`로 지정. 미지정 시 `a.out`
- `&& ./bug` — 컴파일 성공 시에만 실행

```
==36843==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x6020000000f8 at pc 0x000105466a20 bp 0x00016b14a830 sp 0x00016b149fe0
WRITE of size 11 at 0x6020000000f8 thread T0
    #0 0x000105466a1c in strcpy+0x458 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x3aa1c)
    #1 0x000104cb4810 in main bug.c:7
    #2 0x000187c2fdfc in start+0x1b4c (dyld:arm64e+0x1fdfc)

0x6020000000f8 is located 0 bytes after 8-byte region [0x6020000000f0,0x6020000000f8)
allocated by thread T0 here:
    #0 0x00010546d164 in malloc+0x78 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x41164)
    #1 0x000104cb47fc in main bug.c:6

SUMMARY: AddressSanitizer: heap-buffer-overflow bug.c:7 in main
```

읽는 순서

| 항목 | 의미 |
|---|---|
| `heap-buffer-overflow` | 오류 종류 |
| `WRITE of size 11` | 11바이트 쓰기 시도 |
| `#1 ... main bug.c:7` | **오류 발생 위치** (라이브러리 프레임 `#0` 건너뜀) |
| `0 bytes after 8-byte region` | 8바이트 블록 직후 침범 |
| `allocated by ... bug.c:6` | **문제 블록의 할당 위치** |

- 두 행 번호(할당 6행, 오류 7행) 조합으로 원인 즉시 특정

### use-after-free

```c
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    char *p = malloc(16);
    free(p);
    printf("%c\n", p[0]);        // ← 해제된 메모리 읽기
    return 0;
}
```

```bash
cc -g -fsanitize=address uaf.c -o uaf && ./uaf
```

- `-g` — 디버그 심볼 포함. lldb 추적·ASan 행 번호 표시에 필요
- `-fsanitize=address` — AddressSanitizer 활성. 힙 오버플로·use-after-free 즉시 검출
- `-o uaf` — 출력 파일명을 `uaf`로 지정. 미지정 시 `a.out`
- `&& ./uaf` — 컴파일 성공 시에만 실행

```
==36868==ERROR: AddressSanitizer: heap-use-after-free on address 0x6020000000f0 at pc 0x0001040c085c bp 0x00016bd3e800 sp 0x00016bd3e7f8
READ of size 1 at 0x6020000000f0 thread T0
    #0 0x0001040c0858 in main uaf.c:7
    #1 0x000187c2fdfc in start+0x1b4c (dyld:arm64e+0x1fdfc)

0x6020000000f0 is located 0 bytes inside of 16-byte region [0x6020000000f0,0x602000000100)
freed by thread T0 here:
    #0 0x000104831258 in free+0x7c (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x41258)
    #1 0x0001040c0810 in main uaf.c:6
```

- `freed by ... uaf.c:6` — **해제 위치**까지 명시. 8단계 리스트 순회 해제 실수 추적에 유효

### 누수 탐지 — macOS `leaks`

재현 코드 — 포인터 소실로 도달 불가 블록 생성

```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char *g_keep[100];

int main(void) {
    for (int i = 0; i < 100; i++) {
        char *p = malloc(128);      // ← free 누락
        memset(p, 'a', 127);
        p[127] = '\0';
        g_keep[i] = p;
    }
    memset(g_keep, 0, sizeof(g_keep));   // 포인터 소실 → 진짜 누수
    printf("done\n");
    return 0;
}
```

```bash
cc -g leak2.c -o leak2
MallocStackLogging=1 leaks -atExit -- ./leak2 2>&1 | grep -E "leaks for|nodes malloced"
```

- `-g` — 디버그 심볼 포함. lldb 추적·ASan 행 번호 표시에 필요
- `-o leak2` — 출력 파일명을 `leak2`로 지정. 미지정 시 `a.out`

```
Process 36942: 290 nodes malloced for 47 KB
Process 36942: 100 leaks for 16000 total leaked bytes.
```

- 100건 × 128바이트 = 12800바이트. 보고값 16000은 할당자 오버헤드 포함 (블록당 실제 점유 160바이트)
- `MallocStackLogging=1` — 할당 스택 기록 활성화 → 누수 지점 추적 가능
- `-atExit` — 프로세스 종료 직전 검사
- 주의 — 전역 변수가 여전히 포인터를 보유하면 "도달 가능"으로 판정되어 누수 미보고. 위 코드가 `memset`으로 포인터를 지우는 이유

쉘에 적용

```bash
MallocStackLogging=1 leaks -atExit -- ./mysh < test_input.txt 2>&1 | grep "leaks for"
```

- `MallocStackLogging=1` — 할당 스택 기록 활성화 → 누수 발생 지점 추적 가능
- `-atExit` — 프로세스 종료 직전 시점에 누수 검사
- `--` — 이후 인자를 검사 대상 명령으로 전달
- `< test_input.txt` — 파일을 표준 입력으로 공급해 대화형 입력 대체

## lldb 기본 조작

```bash
lldb ./mysh
```

| 명령 | 동작 |
|---|---|
| `b main.c:42` | 파일·행 중단점 |
| `b run_command` | 함수 중단점 |
| `r` | 실행 시작 |
| `n` | 다음 행 (함수 진입 안 함) |
| `s` | 다음 행 (함수 진입) |
| `c` | 계속 실행 |
| `p argv[0]` | 식 평가 |
| `p *hist` | 구조체 내용 출력 |
| `bt` | 스택 추적 |
| `frame variable` | 현재 프레임 변수 전량 |
| `q` | 종료 |

포인터 배열 확인

```
(lldb) p argv[0]
(lldb) p argv[1]
(lldb) memory read --format c --count 32 line
```

자식 프로세스 추적 — `fork` 이후 자식 코드 디버깅

```
(lldb) settings set target.process.follow-fork-mode child
```

- 검증 미완료 — 본 문서 작성 중 미확인. lldb 버전별 지원 여부 확인 필요
- 대안 — 자식 진입 직후 `fprintf(stderr, "child: %s\n", argv[0]);` 삽입. `stderr`는 무버퍼 → 즉시 출력

## 회귀 테스트 스크립트

쉘은 표준 입출력 기반이므로 파이프로 자동 검증 가능

```bash
#!/bin/bash
# test.sh — 기대 출력 비교 방식 회귀 테스트
set -u

PASS=0; FAIL=0

check() {
    local desc="$1" input="$2" expected="$3"
    local actual
    actual=$(printf '%s\n' "$input" | ./mysh 2>&1 | grep -v '^mysh> ')
    if [ "$actual" = "$expected" ]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        printf '실패: %s\n  기대: %s\n  실제: %s\n' "$desc" "$expected" "$actual"
    fi
}

check "echo 기본"    "echo hello"$'\n'"exit"  "hello"
check "파이프"       "printf 'b\na\n' | sort"$'\n'"exit"  $'a\nb'

printf '통과 %d · 실패 %d\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]
```

- 검증 미완료 — 쉘 구현 완성 후 실제 실행 필요. 프롬프트 출력 필터링 방식은 구현에 맞춰 조정
- 프롬프트를 `stderr`로 출력하도록 변경하면 필터링 불필요 → 테스트 용이성 향상

## 단계별 검증 항목 통합

| 단계 | 핵심 확인 | 도구 |
|---|---|---|
| 02 | `read_line` 반환값 전량 해제 | ASan · `leaks` |
| 03 | 배열만 해제, 원소 미해제 | ASan |
| 04 | 좀비 프로세스 부재 | `ps` |
| 05 | `cd` 효과 지속 | `pwd` 수동 확인 |
| 06 | fd 누수 부재 | `lsof -p <pid>` |
| 07 | 데드락 부재 | 실행 후 즉시 종료 확인 |
| 08 | 히스토리 전량 해제 | ASan · `leaks` |
| 09 | 증분 빌드 동작 | `make` 재실행 |

## 함정 · 주의점

- ASan을 릴리스 빌드에 포함 → 성능 대폭 저하. 개발 전용
- `-g` 누락 → ASan 출력에 행 번호 부재 → 추적 난이도 급상승
- `-fsanitize=address`를 컴파일에만 지정 → 링크 오류. 양쪽 필수
- macOS에서 ASan 누수 탐지 기대 → 미지원. `leaks` 사용
- `leaks` 결과가 0건 — 도달 가능 상태이면 누수 미판정. 전역·정적 변수 보유분 주의
- 오류 없이 실행됨 ≠ 정상 — 메모리 오류는 우연히 동작하는 경우 다수. sanitizer 없이 "잘 된다" 판단 금지
- ASan과 `valgrind` 동시 사용 → 충돌. 택일 (macOS arm64에서 valgrind 미지원)
- 자식 프로세스 크래시가 부모에 미전파 → 종료 코드로만 확인 가능. `WIFSIGNALED` 검사

## CLion 팁

- `Run` → `Edit Configurations` → `Sanitizers` 에서 ASan 활성 가능
- CMake 사용 시 Debug 프로필에만 `-fsanitize=address` 적용 권장
- ASan 오류 발생 시 CLion이 스택 추적을 클릭 가능한 링크로 표시
- `Emulate terminal in output console` 활성 → 대화형 입력 및 `Ctrl-C` 테스트 가능
- 메모리 뷰 — 디버거 정지 상태에서 변수 우클릭 → `Show in Memory View`

## 검증

- [ ] ASan 빌드로 전체 시나리오 실행 후 오류 보고 부재
- [ ] `leaks -atExit` 결과 0건
- [ ] `lsof` 확인 시 fd 누적 부재
- [ ] `ps` 확인 시 좀비 프로세스 부재
- [ ] 회귀 스크립트 전 항목 통과
- [ ] lldb로 중단점 설정 및 변수 확인 가능

## 관련 문서

- [[C/projects/make-shell/02-dynamic-input|02 · 동적 입력 버퍼]] — `malloc`·`realloc` 소유권 규약
- [[C/projects/make-shell/09-project-layout|09 · 프로젝트 구조화]] — 헤더 분리와 Make·CMake
- [[C/projects/make-shell/README|로드맵 개요]] — 쉘 구현 10단계 커리큘럼
