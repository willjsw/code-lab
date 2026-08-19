---
tags:
  - topic/os
  - ostep/index
  - reference
aliases:
  - OSTEP 원서 PDF
created: 2026-08-19
updated: 2026-08-19
---

# 원서 PDF 인덱스 (한국어 번역판 v0.91)

> OSTEP 한국어 번역 PDF 53편. 파일명은 공식 저장소 표기 제목 기준, 접두 번호는 책 챕터 번호와 동일

- 출처 — [ostep-translations/korean](https://github.com/remzi-arpacidusseau/ostep-translations/tree/master/korean)
- 배포 원본 — `https://pages.cs.wisc.edu/~remzi/OSTEP/Korean/`
- 파일명 규칙 — `<챕터번호>-<공식 표기 제목>.pdf`. 제목 내 공백은 하이픈, 파일명 사용 불가 문자(`/` `:` `(` `)`)는 제거
- 번호 없는 3편(Preface·Preface-Translate·TOC)은 `00-` 접두

> [!note] PDF 는 git 추적 제외
> 53편 합계 약 19MB 바이너리 → 저장소 경량 유지를 위해 `.gitignore` 로 제외. 새 머신에서는 아래 스크립트로 재수집

```bash
sh download.sh
```

- 옵션 없음. 파트별 디렉토리 생성 후 53편 순차 다운로드
- 이미 존재하는 파일은 `skip` — 중단 후 재실행 시 이어받기 동작
- 완료 후 파일 수와 **PDF 헤더(`%PDF`)를 검증** → HTML 에러 페이지가 저장된 경우 `BAD` 로 표시

```text
skip  03-persistence/50-Summary.pdf

총 53 / 53 편 확보
```

## 00-intro (5편)

| 파일 | 원서 표기 | 주제 |
|---|---|---|
| [00-Preface.pdf](00-intro/00-Preface.pdf) | Preface | 책 구성·서술 장치·프로젝트 안내 |
| [00-Preface-Translate.pdf](00-intro/00-Preface-Translate.pdf) | Preface-Translate | 역자 서문 |
| [00-TOC.pdf](00-intro/00-TOC.pdf) | TOC | 전체 목차 |
| [01-Dialogue.pdf](00-intro/01-Dialogue.pdf) | 1 Dialogue | 책에 관한 대화 — 세 가지 이야기의 유래 |
| [02-Introduction.pdf](00-intro/02-Introduction.pdf) | 2 Introduction | 운영체제 개요 — 가상화·병행성·영속성 맛보기 |

## 01-virtualization (22편)

| 파일 | 원서 표기 | 주제 |
|---|---|---|
| [03-Dialogue.pdf](01-virtualization/03-Dialogue.pdf) | 3 Dialogue | 가상화 대화 — 복숭아 비유 |
| [04-Processes.pdf](01-virtualization/04-Processes.pdf) | 4 Processes | 프로세스 추상 · 상태 · 자료 구조 |
| [05-Process-API.pdf](01-virtualization/05-Process-API.pdf) | 5 Process API | `fork` · `wait` · `exec` |
| [06-Direct-Execution.pdf](01-virtualization/06-Direct-Execution.pdf) | 6 Direct Execution | 제한적 직접 실행 · 모드 전환 · 문맥 교환 |
| [07-CPU-Scheduling.pdf](01-virtualization/07-CPU-Scheduling.pdf) | 7 CPU Scheduling | FIFO · SJF · STCF · RR |
| [08-Multi-level-Feedback.pdf](01-virtualization/08-Multi-level-Feedback.pdf) | 8 Multi-level Feedback | MLFQ 규칙과 개선 3단계 |
| [09-Lottery-Scheduling.pdf](01-virtualization/09-Lottery-Scheduling.pdf) | 9 Lottery Scheduling | 비례 배분 · 추첨 · 보폭 · CFS |
| [10-Multi-CPU-Scheduling.pdf](01-virtualization/10-Multi-CPU-Scheduling.pdf) | 10 Multi-CPU Scheduling | 캐시 일관성 · 친화성 · 단일/다중 큐 |
| [11-Summary.pdf](01-virtualization/11-Summary.pdf) | 11 Summary | CPU 가상화 요약 대화 |
| [12-Dialogue.pdf](01-virtualization/12-Dialogue.pdf) | 12 Dialogue | 메모리 가상화 대화 |
| [13-Address-Spaces.pdf](01-virtualization/13-Address-Spaces.pdf) | 13 Address Spaces | 주소 공간 추상 · 목표 |
| [14-Memory-API.pdf](01-virtualization/14-Memory-API.pdf) | 14 Memory API | `malloc` · `free` · 흔한 오류 |
| [15-Address-Translation.pdf](01-virtualization/15-Address-Translation.pdf) | 15 Address Translation | 베이스·바운드 동적 재배치 |
| [16-Segmentation.pdf](01-virtualization/16-Segmentation.pdf) | 16 Segmentation | 세그멘테이션 · 외부 단편화 |
| [17-Free-Space-Management.pdf](01-virtualization/17-Free-Space-Management.pdf) | 17 Free Space Management | 분할·병합 · best/worst/first fit |
| [18-Introduction-to-Paging.pdf](01-virtualization/18-Introduction-to-Paging.pdf) | 18 Introduction to Paging | 페이지 · 페이지 테이블 · PTE |
| [19-Translation-Lookaside-Buffers.pdf](01-virtualization/19-Translation-Lookaside-Buffers.pdf) | 19 Translation Lookaside Buffers | TLB · 지역성 · 문맥 교환 처리 |
| [20-Advanced-Page-Tables.pdf](01-virtualization/20-Advanced-Page-Tables.pdf) | 20 Advanced Page Tables | 멀티 레벨 · 역페이지 테이블 |
| [21-Swapping-Mechanisms.pdf](01-virtualization/21-Swapping-Mechanisms.pdf) | 21 Swapping: Mechanisms | 스왑 공간 · present 비트 · 페이지 폴트 |
| [22-Swapping-Policies.pdf](01-virtualization/22-Swapping-Policies.pdf) | 22 Swapping: Policies | FIFO · LRU · 클럭 · 스래싱 |
| [23-Case-Study-VAX.pdf](01-virtualization/23-Case-Study-VAX.pdf) | 23 Case Study: VAX | VAX/VMS · 완성형 VM 시스템 |
| [24-Summary.pdf](01-virtualization/24-Summary.pdf) | 24 Summary | 메모리 가상화 요약 대화 |

## 02-concurrency (10편)

| 파일 | 원서 표기 | 주제 |
|---|---|---|
| [25-Dialogue.pdf](02-concurrency/25-Dialogue.pdf) | 25 Dialogue | 병행성 대화 |
| [26-Concurrency-and-Threads.pdf](02-concurrency/26-Concurrency-and-Threads.pdf) | 26 Concurrency and Threads | 스레드 · 경쟁 조건 · 임계 영역 |
| [27-Thread-API.pdf](02-concurrency/27-Thread-API.pdf) | 27 Thread API | `pthread_create` · `join` · `mutex` · `cond` |
| [28-Locks.pdf](02-concurrency/28-Locks.pdf) | 28 Locks | 스핀락 · TAS · CAS · 티켓락 · 2단계 락 |
| [29-Locked-Data-Structures.pdf](02-concurrency/29-Locked-Data-Structures.pdf) | 29 Locked Data Structures | 병행 카운터·리스트·큐·해시 |
| [30-Condition-Variables.pdf](02-concurrency/30-Condition-Variables.pdf) | 30 Condition Variables | 조건 변수 · 생산자·소비자 |
| [31-Semaphores.pdf](02-concurrency/31-Semaphores.pdf) | 31 Semaphores | 세마포어 · 독자·기록자 · 식사하는 철학자 |
| [32-Concurrency-Bugs.pdf](02-concurrency/32-Concurrency-Bugs.pdf) | 32 Concurrency Bugs | 원자성·순서 위반 · 교착 상태 4조건 |
| [33-Event-based-Concurrency.pdf](02-concurrency/33-Event-based-Concurrency.pdf) | 33 Event-based Concurrency | 이벤트 루프 · `select`/`poll` · 비동기 I/O |
| [34-Summary.pdf](02-concurrency/34-Summary.pdf) | 34 Summary | 병행성 요약 대화 |

## 03-persistence (16편)

| 파일 | 원서 표기 | 주제 |
|---|---|---|
| [35-Dialogue.pdf](03-persistence/35-Dialogue.pdf) | 35 Dialogue | 영속성 대화 |
| [36-IO-Devices.pdf](03-persistence/36-IO-Devices.pdf) | 36 I/O Devices | 버스 · 폴링 vs 인터럽트 · DMA · 디바이스 드라이버 |
| [37-Hard-Disk-Drives.pdf](03-persistence/37-Hard-Disk-Drives.pdf) | 37 Hard Disk Drives | 탐색·회전·전송 시간 · 디스크 스케줄링 |
| [38-Redundant-Disk-Arrays-RAID.pdf](03-persistence/38-Redundant-Disk-Arrays-RAID.pdf) | 38 Redundant Disk Arrays (RAID) | RAID 0·1·4·5 비교 |
| [39-Files-and-Directories.pdf](03-persistence/39-Files-and-Directories.pdf) | 39 Files and Directories | 파일 디스크립터 · `open`/`read`/`write` · 하드·심볼릭 링크 |
| [40-File-System-Implementation.pdf](03-persistence/40-File-System-Implementation.pdf) | 40 File System Implementation | inode · 데이터 블록 · 비트맵 · 읽기·쓰기 경로 |
| [41-Fast-File-System-FFS.pdf](03-persistence/41-Fast-File-System-FFS.pdf) | 41 Fast File System (FFS) | 실린더 그룹 · 지역성 배치 정책 |
| [42-FSCK-and-Journaling.pdf](03-persistence/42-FSCK-and-Journaling.pdf) | 42 FSCK and Journaling | 크래시 일관성 · 저널링 3단계 |
| [43-Log-Structured-File-System-LFS.pdf](03-persistence/43-Log-Structured-File-System-LFS.pdf) | 43 Log-Structured File System (LFS) | 순차 쓰기 · inode 맵 · 세그먼트 클리닝 |
| [44-Data-Integrity-and-Protection.pdf](03-persistence/44-Data-Integrity-and-Protection.pdf) | 44 Data Integrity and Protection | 잠재적 오류 · 체크섬 · 스크러빙 |
| [45-Summary.pdf](03-persistence/45-Summary.pdf) | 45 Summary | 영속성 요약 대화 |
| [46-Dialogue.pdf](03-persistence/46-Dialogue.pdf) | 46 Dialogue | 분산 시스템 대화 |
| [47-Distributed-Systems.pdf](03-persistence/47-Distributed-Systems.pdf) | 47 Distributed Systems | 통신 신뢰성 · UDP/TCP · RPC |
| [48-Network-File-System-NFS.pdf](03-persistence/48-Network-File-System-NFS.pdf) | 48 Network File System (NFS) | 무상태 프로토콜 · 멱등 연산 · 캐시 일관성 |
| [49-Andrew-File-System-AFS.pdf](03-persistence/49-Andrew-File-System-AFS.pdf) | 49 Andrew File System (AFS) | 전체 파일 캐싱 · 콜백 |
| [50-Summary.pdf](03-persistence/50-Summary.pdf) | 50 Summary | 분산 요약 대화 |

## 관련 문서

- [[OS/ostep/README|OSTEP 학습 진입점]] — 전체 구성·학습 순서
- [[OS/ostep/docs/README|이론 문서 인덱스]] — 각 PDF에 대응하는 정리 문서
