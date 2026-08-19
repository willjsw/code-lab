---
tags:
  - topic/os
  - ostep/index
  - moc
aliases:
  - OSTEP 이론 문서 인덱스
created: 2026-08-19
updated: 2026-08-19
---

# 이론 문서 인덱스

> OSTEP 전 50챕터 이론 정리. 문서 번호·절 번호는 원서 챕터와 1:1 대응

- 문서 1편 = 원서 1챕터. 문서 내 `## N.M` 절 번호도 원서와 동일
- 각 문서 상단에 원서 PDF 링크·대응 실습 링크 배치
- 진행 상태 — `완료` 작성·검증 완료 / `예정` 미작성

## 파트 0 — 서론 (`00-intro/`)

| 챕터 | 문서 | 상태 | 핵심 |
|---|---|---|---|
| Preface | [00-preface.md](00-intro/00-preface.md) | 완료 | 책 구성·서술 장치·학습 방법 |
| 1 | [01-dialogue.md](00-intro/01-dialogue.md) | 완료 | 세 가지 이야기의 유래 |
| 2 | [02-introduction.md](00-intro/02-introduction.md) | 완료 | 가상화·병행성·영속성 개요, 설계 목표 |

## 파트 1 — 가상화 (`01-virtualization/`)

### CPU 가상화 (ch 3~11)

| 챕터 | 문서 | 상태 | 핵심 |
|---|---|---|---|
| 3 | [03-dialogue.md](01-virtualization/03-dialogue.md) | 완료 | 복숭아 비유로 본 가상화 |
| 4 | [04-processes.md](01-virtualization/04-processes.md) | 완료 | 프로세스 추상·상태 전이·PCB |
| 5 | [05-process-api.md](01-virtualization/05-process-api.md) | 완료 | `fork` · `wait` · `exec` 조합 |
| 6 | [06-direct-execution.md](01-virtualization/06-direct-execution.md) | 완료 | 제한적 직접 실행·트랩·문맥 교환 |
| 7 | [07-cpu-scheduling.md](01-virtualization/07-cpu-scheduling.md) | 완료 | FIFO·SJF·STCF·RR, 반환·응답 시간 |
| 8 | [08-multi-level-feedback.md](01-virtualization/08-multi-level-feedback.md) | 완료 | MLFQ 규칙 5개와 개선 3단계 |
| 9 | [09-lottery-scheduling.md](01-virtualization/09-lottery-scheduling.md) | 완료 | 추첨·보폭 스케줄링, CFS |
| 10 | [10-multi-cpu-scheduling.md](01-virtualization/10-multi-cpu-scheduling.md) | 완료 | 캐시 친화성, 단일 큐 vs 다중 큐 |
| 11 | [11-summary.md](01-virtualization/11-summary.md) | 완료 | CPU 가상화 요약 |

### 메모리 가상화 (ch 12~24)

| 챕터 | 문서 | 상태 | 핵심 |
|---|---|---|---|
| 12 | [12-dialogue.md](01-virtualization/12-dialogue.md) | 완료 | 메모리 가상화 도입 대화 |
| 13 | [13-address-spaces.md](01-virtualization/13-address-spaces.md) | 완료 | 주소 공간 추상, 투명성·효율·보호 |
| 14 | [14-memory-api.md](01-virtualization/14-memory-api.md) | 완료 | `malloc`/`free`, 대표 메모리 오류 6종 |
| 15 | [15-address-translation.md](01-virtualization/15-address-translation.md) | 완료 | 베이스·바운드, MMU |
| 16 | [16-segmentation.md](01-virtualization/16-segmentation.md) | 완료 | 세그먼트별 베이스·바운드, 외부 단편화 |
| 17 | [17-free-space-management.md](01-virtualization/17-free-space-management.md) | 완료 | 프리 리스트, 분할·병합, fit 정책 |
| 18 | [18-introduction-to-paging.md](01-virtualization/18-introduction-to-paging.md) | 완료 | 페이지·프레임, 선형 페이지 테이블 |
| 19 | [19-tlb.md](01-virtualization/19-tlb.md) | 완료 | TLB 알고리즘, 지역성, ASID |
| 20 | `20-advanced-page-tables.md` | 예정 | 멀티 레벨 페이지 테이블 |
| 21 | `21-swapping-mechanisms.md` | 예정 | 스왑 공간, present 비트, 페이지 폴트 |
| 22 | `22-swapping-policies.md` | 예정 | 최적·FIFO·LRU·클럭, 스래싱 |
| 23 | `23-case-study-vax.md` | 예정 | VAX/VMS 종합 사례 |
| 24 | `24-summary.md` | 예정 | 메모리 가상화 요약 |

## 파트 2 — 병행성 (`02-concurrency/`)

| 챕터 | 문서 | 상태 | 핵심 |
|---|---|---|---|
| 25 | `25-dialogue.md` | 예정 | 병행성 도입 대화 |
| 26 | `26-concurrency-and-threads.md` | 예정 | 스레드, 경쟁 조건, 임계 영역 |
| 27 | `27-thread-api.md` | 예정 | `pthread` 생성·조인·뮤텍스·조건 변수 |
| 28 | `28-locks.md` | 예정 | TAS·CAS·티켓락·2단계 락 |
| 29 | `29-locked-data-structures.md` | 예정 | 병행 카운터·리스트·큐·해시 |
| 30 | `30-condition-variables.md` | 예정 | 조건 변수, 생산자·소비자 |
| 31 | `31-semaphores.md` | 예정 | 세마포어, 독자·기록자, 철학자 |
| 32 | `32-concurrency-bugs.md` | 예정 | 원자성·순서 위반, 교착 상태 4조건 |
| 33 | `33-event-based-concurrency.md` | 예정 | 이벤트 루프, `select`/`poll` |
| 34 | `34-summary.md` | 예정 | 병행성 요약 |

## 파트 3 — 영속성 (`03-persistence/`)

| 챕터 | 문서 | 상태 | 핵심 |
|---|---|---|---|
| 35 | `35-dialogue.md` | 예정 | 영속성 도입 대화 |
| 36 | `36-io-devices.md` | 예정 | 폴링 vs 인터럽트, DMA, 드라이버 |
| 37 | `37-hard-disk-drives.md` | 예정 | 탐색·회전·전송, SSTF·SCAN |
| 38 | `38-raid.md` | 예정 | RAID 0·1·4·5 용량·신뢰성·성능 |
| 39 | `39-files-and-directories.md` | 예정 | 파일 디스크립터, 링크, `unlink` |
| 40 | `40-file-system-implementation.md` | 예정 | inode·비트맵·읽기·쓰기 경로 |
| 41 | `41-ffs.md` | 예정 | 실린더 그룹, 지역성 배치 |
| 42 | `42-fsck-and-journaling.md` | 예정 | 크래시 일관성, 저널링 3단계 |
| 43 | `43-lfs.md` | 예정 | 순차 쓰기, inode 맵, 클리닝 |
| 44 | `44-data-integrity.md` | 예정 | 잠재적 오류, 체크섬, 스크러빙 |
| 45 | `45-summary.md` | 예정 | 영속성 요약 |
| 46 | `46-dialogue.md` | 예정 | 분산 도입 대화 |
| 47 | `47-distributed-systems.md` | 예정 | UDP/TCP, RPC, 통신 실패 |
| 48 | `48-nfs.md` | 예정 | 무상태 프로토콜, 멱등성 |
| 49 | `49-afs.md` | 예정 | 전체 파일 캐싱, 콜백 |
| 50 | `50-summary.md` | 예정 | 분산 요약 |

## 문서 규약

- 문체 — 명사형 개조체. 종결어미 미사용
- 코드 — 컴파일 가능한 완결 형태. 컴파일 명령·옵션 설명·실제 출력 병기
- 다이어그램 — 구조·시간 전개는 Mermaid 필수 (메모리 배치·상태 전이·타임라인)
- 서술 장치 — 원서의 `CRUX`·`ASIDE`·`TIP` 을 Obsidian 콜아웃으로 대응
- 환경 차이 — macOS·arm64 에서 책과 결과가 다른 경우 해당 절에 명시

## 관련 문서

- [[OS/ostep/README|OSTEP 학습 진입점]] — 전체 구성·학습 순서·측정 환경
- [[OS/ostep/projects/README|실습 프로젝트 인덱스]] — 각 챕터 대응 코드 실습
- [[OS/ostep/pdfs/README|원서 PDF 인덱스]] — 챕터별 원문
