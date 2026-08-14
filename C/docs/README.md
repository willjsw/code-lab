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

작성 예정. 현재는 [make-shell 02단계](../projects/make-shell/02-dynamic-input.md) 참조

### 03-build — 빌드

| 문서 | 내용 |
|---|---|
| [gcc 컴파일 · 실행 명령어](03-build/gcc-compile-and-run.md) | 필수 옵션, 다중 파일, 라이브러리 링크, 오류 해석 |
| [Makefile 작성법](03-build/makefile-guide.md) | 규칙·변수·자동 변수·패턴 규칙, 헤더 의존성 추적 |
| [빌드 산출물 정리](03-build/build-artifacts-cleanup.md) | `.o`·`a.out`·`.dSYM` 종류, 수동 삭제 주의점, `.gitignore` |

### 04-project-layout — 프로젝트 구조

| 문서 | 내용 |
|---|---|
| [C 소스코드 구성 요소](04-project-layout/source-file-types.md) | `.c`·`.h`·`.o`·`.a`·`.dylib`, 선언·정의 분리, `static` |

### 05-debugging — 디버깅

작성 예정. 현재는 [make-shell 10단계](../projects/make-shell/10-debugging.md) 참조

### 06-system — 시스템 프로그래밍

작성 예정. 현재는 [POSIX 시스템 호출](07-stdlib/05-posix.md) 및 [make-shell](../projects/make-shell/README.md) 참조

### 07-stdlib — 표준 라이브러리

| 문서 | 헤더 |
|---|---|
| [시리즈 개요 · 빈출 함수 30선 · 통합 예제](07-stdlib/README.md) | — |
| [01 표준 입출력](07-stdlib/01-stdio.md) | `<stdio.h>` |
| [02 문자열 처리](07-stdlib/02-string.md) | `<string.h>` |
| [03 메모리 · 변환 · 유틸리티](07-stdlib/03-stdlib.md) | `<stdlib.h>` |
| [04 문자 · 수학 · 시간](07-stdlib/04-ctype-math-time.md) | `<ctype.h>` `<math.h>` `<time.h>` `<limits.h>` 외 |
| [05 POSIX 시스템 호출](07-stdlib/05-posix.md) | `<unistd.h>` `<fcntl.h>` `<sys/*.h>` |

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
| `printf` 서식 | [표준 입출력](07-stdlib/01-stdio.md) |
| 문자열 다루기 | [문자열 처리](07-stdlib/02-string.md) |
| `malloc` 사용법 | [메모리 · 변환](07-stdlib/03-stdlib.md) |
| 메모리 누수 찾기 | [make-shell 10단계](../projects/make-shell/10-debugging.md) |
| 파일 읽기쓰기 | [표준 입출력](07-stdlib/01-stdio.md) · [POSIX](07-stdlib/05-posix.md) |
| 프로세스 실행 | [POSIX](07-stdlib/05-posix.md) · [make-shell 04단계](../projects/make-shell/04-process-exec.md) |

## 문서 규약

- 전 예제는 macOS(arm64)에서 실제 컴파일·실행 검증 후 출력 기재
- 검증하지 못한 내용은 `검증 미완료` 명시
- Linux와 동작이 다른 지점은 문서 내 별도 표기
