---
tags:
  - lang/c
  - c/index
  - index
  - moc
  - status/wip
aliases:
  - C 학습 인덱스
created: 2026-08-14
updated: 2026-08-18
---

# C 학습 문서 인덱스

> Java 기반 개발자의 C·시스템 프로그래밍 학습 기록

## 시작 경로

C 기본 문법만 아는 상태에서 **CLI 프로그램을 직접 컴파일·실행**하기까지의 최단 경로

1. [C 프로그램의 동작 및 컴파일 방식](01-basics/c-program-execution-model.md) — 소스가 실행 파일이 되는 과정
2. [gcc 컴파일 · 실행 명령어](03-build/gcc-compile-and-run.md) — 실제 빌드 명령
3. [C 소스코드 구성 요소](04-project-layout/source-file-types.md) — `.c`·`.h`·`.o`·`.a` 역할
4. [Makefile 작성법](03-build/makefile-guide.md) — 빌드 자동화
5. [표준 라이브러리 시리즈](07-stdlib/README.md) — 실제로 쓸 함수들

## 카테고리별 문서

### 01-basics — 기초

| 문서 | 내용 |
|---|---|
| [C 프로그램의 동작 및 컴파일 방식](01-basics/c-program-execution-model.md) | 컴파일 4단계, 메모리 세그먼트, `main` 진입·종료 |

### 02-memory — 메모리

| 문서 | 내용 |
|---|---|
| [`free`의 실제 동작](02-memory/heap-and-free.md) | 할당자 3계층, OS 미반환, 블록 재사용, 댕글링 포인터, ASan 검출 |

동적 입력 버퍼 실전 적용은 [make-shell 02단계](../projects/make-shell/02-dynamic-input.md) 참조

### 03-build — 빌드

| 문서 | 내용 |
|---|---|
| [gcc 컴파일 · 실행 명령어](03-build/gcc-compile-and-run.md) | 필수 옵션, 다중 파일, 라이브러리 링크, 오류 해석 |
| [Makefile 작성법](03-build/makefile-guide.md) | 규칙·변수·자동 변수·패턴 규칙, 헤더 의존성 추적 |
| [CMakeLists.txt 작성법](03-build/cmake-guide.md) | CMake 기본 명령, out-of-source 빌드, CLion 연동, Makefile 병행 충돌 |
| [빌드 산출물 정리](03-build/build-artifacts-cleanup.md) | `.o`·`a.out`·`.dSYM` 종류, 수동 삭제 주의점, `.gitignore` |

### 04-project-layout — 프로젝트 구조

| 문서 | 내용 |
|---|---|
| [C 소스코드 구성 요소](04-project-layout/source-file-types.md) | `.c`·`.h`·`.o`·`.a`·`.dylib`, 선언·정의 분리, `static` |

### 05-debugging — 디버깅

| 문서 | 내용 |
|---|---|
| [lldb로 메모리 주소 값 조회하기](05-debugging/lldb-memory-inspection.md) | CLion 디버거 구조, `memory read`, watchpoint, 리틀 엔디안, 구조체 패딩 |

ASan·`leaks` 실전 적용은 [make-shell 10단계](../projects/make-shell/10-debugging.md) 참조

### 06-system — 시스템 프로그래밍

작성 예정. 현재는 [POSIX 시스템 호출](07-stdlib/05-posix.md) 및 [make-shell](../projects/make-shell/README.md) 참조

### 08-syntax — 연산자 · 특수 문법

| 문서 | 내용 |
|---|---|
| [전처리기 매크로](08-syntax/preprocessor-macro.md) | `#define` 텍스트 치환, 괄호 함정, 조건부 컴파일 |
| [sizeof 연산자와 배열 첨자](08-syntax/sizeof-and-array-subscript.md) | `sizeof` 컴파일 시점 평가, 배열 감쇠, 포인터 산술 |
| [static 키워드](08-syntax/static-keyword.md) | 링키지·저장 기간, 정적 지역변수, **Java `static`과 의미 전도** |
| [size_t 타입](08-syntax/size-t-type.md) | 부호 없는 크기 타입, 언더플로, `%zu`, `ssize_t` |
| [문자 리터럴과 문자열 리터럴](08-syntax/character-literal.md) | `'x'`는 `int` 상수, `"x"`는 주소, 이스케이프 시퀀스 |
| [이중 포인터 `**`](08-syntax/double-pointer.md) | out-parameter, `char **argv`, 성장 버퍼, 연결 리스트, 2차원 배열 |

### 07-stdlib — 표준 라이브러리

| 문서 | 헤더 |
|---|---|
| [시리즈 개요 · 빈출 함수 30선 · 통합 예제](07-stdlib/README.md) | — |
| [01 표준 입출력](07-stdlib/01-stdio.md) | `<stdio.h>` |
| [02 문자열 처리](07-stdlib/02-string.md) | `<string.h>` |
| [03 메모리 · 변환 · 유틸리티](07-stdlib/03-stdlib.md) | `<stdlib.h>` |
| [04 문자 · 수학 · 시간](07-stdlib/04-ctype-math-time.md) | `<ctype.h>` `<math.h>` `<time.h>` `<limits.h>` 외 |
| [05 POSIX 시스템 호출](07-stdlib/05-posix.md) | `<unistd.h>` `<fcntl.h>` `<sys/*.h>` |
| [06 표준 입출력 버퍼링](07-stdlib/06-stdio-buffering.md) | `<stdio.h>` 심화 — `fflush`·`setvbuf` |

## 프로젝트

| 프로젝트 | 내용 |
|---|---|
| [make-shell](../projects/make-shell/README.md) | C 쉘 구현 10단계 커리큘럼. 메모리·프로세스·fd·시그널·빌드·디버깅 통합 학습 |

### make-shell 단계별 문서

| 단계 | 문서 | 핵심 주제 |
|---|---|---|
| 01 | [REPL 골격](../projects/make-shell/01-repl-skeleton.md) | 무한 루프, 표준 입출력, `fflush` |
| 02 | [동적 입력 버퍼](../projects/make-shell/02-dynamic-input.md) | `malloc`·`realloc`, 소유권 규약 |
| 03 | [토크나이저](../projects/make-shell/03-tokenizer.md) | `strtok_r`, 이중 포인터, NULL 종단 |
| 04 | [프로세스 실행](../projects/make-shell/04-process-exec.md) | `fork`·`execvp`·`waitpid` |
| 05 | [내장 명령](../projects/make-shell/05-builtins.md) | `chdir`, 프로세스 상태 격리 |
| 06 | [리다이렉션](../projects/make-shell/06-redirection.md) | `open`·`dup2`, 파일 디스크립터 |
| 07 | [파이프](../projects/make-shell/07-pipes.md) | `pipe`, fd 닫기 규율, 데드락 |
| 08 | [시그널 · 히스토리](../projects/make-shell/08-signals-history.md) | `sigaction`, 연결 리스트 |
| 09 | [프로젝트 구조화](../projects/make-shell/09-project-layout.md) | 헤더 분리, Make·CMake |
| 10 | [디버깅 · 검증](../projects/make-shell/10-debugging.md) | ASan, `leaks`, lldb |

## 주제별 빠른 참조

| 알고 싶은 것 | 문서 |
|---|---|
| `gcc` 명령이 하는 일 | [컴파일 4단계](01-basics/c-program-execution-model.md) |
| 컴파일 오류 vs 링크 오류 | [gcc 명령어 — 오류 해석](03-build/gcc-compile-and-run.md) |
| 빌드 후 남은 파일 지우기 | [빌드 산출물 정리](03-build/build-artifacts-cleanup.md) |
| 소스 수정이 반영 안 될 때 | [빌드 산출물 정리 — stale `.o`](03-build/build-artifacts-cleanup.md) |
| 헤더에 뭘 쓰나 | [소스코드 구성 요소](04-project-layout/source-file-types.md) |
| 파일 여러 개로 나누기 | [소스코드 구성 요소](04-project-layout/source-file-types.md) · [Makefile](03-build/makefile-guide.md) |
| CLion이 쓰는 빌드 설정 | [CMakeLists.txt 작성법](03-build/cmake-guide.md) |
| Makefile과 CMake 같이 쓸 때 문제 | [CMakeLists.txt — 충돌 4종](03-build/cmake-guide.md) |
| `printf` 서식 | [표준 입출력](07-stdlib/01-stdio.md) |
| `printf` 출력이 안 나오거나 순서가 뒤바뀔 때 | [표준 입출력 버퍼링](07-stdlib/06-stdio-buffering.md) |
| 프롬프트가 입력 후에 뜰 때 | [버퍼링 — 프롬프트 실험](07-stdlib/06-stdio-buffering.md) |
| `fork` 후 출력이 두 번 찍힐 때 | [버퍼링 — `fork` 버퍼 복제](07-stdlib/06-stdio-buffering.md) |
| 문자열 다루기 | [문자열 처리](07-stdlib/02-string.md) |
| `malloc` 사용법 | [메모리 · 변환](07-stdlib/03-stdlib.md) |
| `#define` 매크로 주의점 | [전처리기 매크로](08-syntax/preprocessor-macro.md) |
| `sizeof`가 이상한 값을 줄 때 | [sizeof와 배열 첨자](08-syntax/sizeof-and-array-subscript.md) |
| `static`이 Java와 뭐가 다른지 | [static 키워드](08-syntax/static-keyword.md) |
| 뺄셈 결과가 거대한 양수로 나올 때 | [size_t 타입 — 언더플로](08-syntax/size-t-type.md) |
| `for` 역순 순회가 안 끝날 때 | [size_t 타입 — 역순 순회](08-syntax/size-t-type.md) |
| `fgetc` 읽기가 중간에 끊길 때 | [표준 입출력 — `EOF`와 반환 타입](07-stdlib/01-stdio.md) |
| `'x'`와 `"x"`의 차이 | [문자 리터럴과 문자열 리터럴](08-syntax/character-literal.md) |
| `int` 변수를 `'\n'`과 비교해도 되는지 | [문자 리터럴 — 정수 비교](08-syntax/character-literal.md) |
| `**`가 왜 필요한지 | [이중 포인터](08-syntax/double-pointer.md) |
| 함수에서 할당했는데 호출자가 `NULL`일 때 | [이중 포인터 — out-parameter](08-syntax/double-pointer.md) |
| `char **argv` 구조 | [이중 포인터 — 문자열 배열](08-syntax/double-pointer.md) · [make-shell 03단계](../projects/make-shell/03-tokenizer.md) |
| 메모리 누수 찾기 | [make-shell 10단계](../projects/make-shell/10-debugging.md) |
| `free` 후 메모리에 무슨 일이 일어나는지 | [`free`의 실제 동작](02-memory/heap-and-free.md) |
| `free` 했는데 메모리 사용량이 안 줄 때 | [`free`의 실제 동작 — OS 반환 여부](02-memory/heap-and-free.md) |
| `free` 후 접근이 우연히 되는 이유 | [`free`의 실제 동작 — 댕글링 포인터](02-memory/heap-and-free.md) |
| 메모리 주소에 든 실제 바이트 보기 | [lldb 메모리 조회](05-debugging/lldb-memory-inspection.md) |
| 변수 값이 언제 바뀌는지 추적 | [lldb 메모리 조회 — watchpoint](05-debugging/lldb-memory-inspection.md) |
| 구조체 패딩이 얼마나 붙는지 | [lldb 메모리 조회 — 구조체 패딩](05-debugging/lldb-memory-inspection.md) |
| 파일 읽기쓰기 | [표준 입출력](07-stdlib/01-stdio.md) · [POSIX](07-stdlib/05-posix.md) |
| 프로세스 실행 | [POSIX](07-stdlib/05-posix.md) · [make-shell 04단계](../projects/make-shell/04-process-exec.md) |

## 문서 규약

- 전 예제는 macOS(arm64)에서 실제 컴파일·실행 검증 후 출력 기재
- 검증하지 못한 내용은 `검증 미완료` 명시
- Linux와 동작이 다른 지점은 문서 내 별도 표기

## 관련 문서

- [[C/docs/01-basics/c-program-execution-model|C 프로그램의 동작 및 컴파일 방식]] — 소스가 실행 파일이 되는 과정
- [[C/docs/02-memory/heap-and-free|free의 실제 동작]] — 힙 할당자 구조와 해제 후 메모리 상태
- [[C/docs/05-debugging/lldb-memory-inspection|lldb로 메모리 주소 값 조회하기]] — CLion 디버거로 실제 바이트 확인
- [[C/docs/03-build/gcc-compile-and-run|gcc 컴파일 · 실행 명령어]] — 컴파일 명령과 옵션 전반
- [[C/docs/03-build/makefile-guide|Makefile 작성법]] — 빌드 자동화와 증분 빌드
- [[C/docs/03-build/cmake-guide|CMakeLists.txt 작성법]] — CMake 문법·out-of-source 빌드·Makefile 병행 충돌
- [[C/docs/03-build/build-artifacts-cleanup|빌드 산출물 정리]] — 빌드 산출물 정리와 `.gitignore`
- [[C/docs/04-project-layout/source-file-types|C 소스코드 구성 요소]] — `.c`·`.h`·`.o`·`.a` 파일 역할
- [[C/docs/07-stdlib/README|표준 라이브러리 시리즈]] — 빈출 함수 30선과 통합 예제
- [[C/docs/07-stdlib/06-stdio-buffering|표준 입출력 버퍼링과 fflush]] — 버퍼링 모드와 플러시 시점 제어
- [[C/docs/08-syntax/preprocessor-macro|전처리기 매크로]] — `#define` 치환과 괄호 함정
- [[C/docs/08-syntax/sizeof-and-array-subscript|sizeof 연산자와 배열 첨자]] — 배열 감쇠와 포인터 산술
- [[C/docs/08-syntax/static-keyword|static 키워드]] — 링키지·저장 기간과 Java `static`과의 의미 차이
- [[C/docs/08-syntax/size-t-type|size_t 타입]] — 부호 없는 크기 타입과 언더플로 함정
- [[C/docs/08-syntax/character-literal|문자 리터럴과 문자열 리터럴]] — `'x'`의 타입이 `int`인 이유와 이스케이프 시퀀스
- [[C/docs/08-syntax/double-pointer|이중 포인터]] — 포인터 자체를 바꿔야 할 때의 유일한 수단
- [[C/projects/make-shell/README|make-shell 프로젝트]] — 쉘 구현 10단계 커리큘럼
