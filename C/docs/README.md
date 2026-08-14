# C 학습 문서 인덱스

> Java 기반 개발자의 C·시스템 프로그래밍 학습 기록

## 카테고리

| 디렉토리 | 범위 |
|---|---|
| `01-basics/` | 문법, 타입, 전처리기 |
| `02-memory/` | 포인터, 스택·힙, `malloc`, 누수 |
| `03-build/` | 컴파일·링크, 컴파일러 옵션, Make·CMake |
| `04-project-layout/` | 디렉토리 구조, 헤더 분리, 코드 컨벤션 |
| `05-debugging/` | gdb·lldb, CLion 디버거, sanitizer |
| `06-system/` | 파일 I/O, 프로세스, 스레드, 시스템 콜 |

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

## 개념 문서

작성된 개념 문서 부재. 학습 진행에 따라 카테고리별 추가 예정
