---
tags:
  - topic/os
  - ostep/virtualization
  - lang/c
  - project/ostep-05
  - allocator
  - free-list
  - fragmentation
  - mmap
  - status/verified
aliases:
  - OSTEP 실습 5
  - 할당기 구현 실습
created: 2026-08-19
updated: 2026-08-19
---

# 실습 5. 프리 리스트 할당기 직접 구현 (ch17)

> `malloc`/`free` 를 직접 만들어 헤더·프리 리스트 임베딩·분할·병합·fit 정책 3종을 동작으로 확인

- 이론 — [[OS/ostep/docs/01-virtualization/17-free-space-management|17. 프리 공간 관리]]
- 원서 PDF — [17-Free-Space-Management.pdf](../../pdfs/01-virtualization/17-Free-Space-Management.pdf)
- 원서 공식 코드 없음 — 원서 17.2·17.3 설명을 그대로 구현한 것

## 구현 요소

| 요소 | 원서 대응 | 구현 내용 |
|---|---|---|
| 힙 확보 | 17.2 Embedding A Free List | `mmap` 으로 4096바이트 익명 매핑 |
| 헤더 | 17.2 Tracking The Size | `header_t { int size; int magic; }` — `free` 가 크기를 알아내는 수단 |
| 프리 리스트 임베딩 | 17.2 Embedding A Free List | `node_t { int size; node_t *next; }` 를 **빈 공간 안에** 직접 배치 |
| 분할 | 17.2 Splitting | 남는 부분이 `node_t` 를 담을 만큼 크면 리스트에 유지 |
| 병합 | 17.2 Coalescing | 주소 순 삽입 후 앞·뒤 이웃과 인접 여부 검사 |
| fit 정책 | 17.3 Basic Strategies | best / worst / first 선택 가능 |

## 빌드 · 실행

```bash
make
./myalloc
```

- `make` — 옵션 없음. `myalloc` 빌드
- `./myalloc` — 옵션 없음. 3개 데모 순차 실행

```text
gcc -Wall -Wextra -Wno-unused-parameter -g -o myalloc myalloc.c
```

- `-Wall` — 주요 경고 활성
- `-Wextra` — 추가 경고 활성
- `-Wno-unused-parameter` — `main(int argc, char *argv[])` 인자 미사용 경고 비활성
- `-g` — 디버그 심볼 포함
- `-o myalloc` — 출력 실행 파일명 지정

## 실행 결과 (macOS arm64 실측)

```text
sizeof(header_t) = 8, sizeof(node_t) = 16
```

- **헤더 8바이트** — `int size` + `int magic`. 원서 가정과 일치
- **노드 16바이트** — `int size`(4) + `node_t *next`(8) = 12 이지만, arm64 는 포인터 정렬(8바이트) 때문에 **16바이트로 패딩**. 원서는 32비트 가정으로 8바이트 → **초기 프리 청크가 원서의 4088 이 아니라 4080**

### 1. 분할과 병합

```text
===== 1. 분할과 병합 =====
초기 상태                [off:0 len:4080]   (청크 1개, 빈 공간 4080바이트)
my_malloc(100) 후           [off:108 len:3972]   (청크 1개, 빈 공간 3972바이트)
my_malloc(100) 후           [off:216 len:3864]   (청크 1개, 빈 공간 3864바이트)
my_malloc(100) 후           [off:324 len:3756]   (청크 1개, 빈 공간 3756바이트)
가운데 블록 해제 후  [off:108 len:108] -> [off:324 len:3756]   (청크 2개, 빈 공간 3864바이트)
첫 블록 해제 후 (병합) [off:0 len:216] -> [off:324 len:3756]   (청크 2개, 빈 공간 3972바이트)
마지막 해제 후 (전부 병합) [off:0 len:4080]   (청크 1개, 빈 공간 4080바이트)
```

**검산 — 원서 17.2 와 대조**

| 단계 | 계산 | 실측 | 원서(32비트 가정) |
|---|---|---|---|
| 초기 | `4096 − sizeof(node_t)` = `4096 − 16` | **4080** | 4088 (노드 8바이트) |
| `malloc(100)` 1회 | 소비 = `100 + 8`(헤더) = 108 → `4080 − 108` | **3972** | 3980 |
| `malloc(100)` 3회 | 소비 = `108 × 3` = 324 → `4080 − 324` | **3756** | 3764 |

- 오프셋 108·216·324 가 **정확히 108바이트 간격** → 헤더 8 + 사용자 100
- **가운데 해제 시** — 프리 청크 2개로 나뉘며 병합되지 않음. 이웃이 사용 중이므로 정상. 원서 Figure 17.6 과 동일 상황
- **첫 블록 해제 시** — `[off:0 len:108]` 이 `[off:108 len:108]` 과 인접 → **`[off:0 len:216]` 으로 병합** ✓
- **마지막 해제 시** — 세 조각이 전부 이어져 **`[off:0 len:4080]` 으로 완전 복원** ✓ → 원서 17.2 "the heap will be whole again"

### 2. fit 정책 3종 비교

세 정책 모두 **동일한 프리 리스트**에서 시작.

```text
요청 전 프리 리스트  [off:208 len:38] -> [off:454 len:18] -> [off:680 len:3400]
```

| 정책 | 선택한 청크 | 반환 오프셋 | 요청 후 프리 리스트 | 근거 |
|---|---|---|---|---|
| **best fit** | `len:18` (두 번째) | **462** | `[off:208 len:38] -> [off:680 len:3400]` | **가장 작은 적합** 청크. 18 − 16 = 2 < 노드 16 → 청크 전체 소비 |
| **worst fit** | `len:3400` (세 번째) | **688** | `[off:208 len:38] -> [off:454 len:18] -> [off:696 len:3384]` | **가장 큰** 청크. 큰 청크를 남기려는 의도와 반대로 큰 청크를 깎음 |
| **first fit** | `len:38` (첫 번째) | **216** | `[off:224 len:22] -> [off:454 len:18] -> [off:680 len:3400]` | **첫 적합**. 전체 탐색 불필요 → 가장 빠름 |

**관찰 포인트**

- 세 정책이 **각기 다른 청크**를 선택 → 원서 17.3 예제(10 / 30 / 20 에 15 요청)의 재현
- **best fit** — 딱 맞는 청크를 소비해 프리 리스트 항목 수를 **3 → 2 로 줄임**. 다만 남은 2바이트는 회수 불가 → **내부 단편화**
- **worst fit** — 큰 청크를 깎아 **작은 청크들이 그대로 남음**. 원서 지적대로 "대부분의 연구가 나쁜 성능을 보인다"
- **first fit** — 첫 청크를 분할해 **작은 조각(22바이트)을 리스트 앞쪽에 남김** → 원서 지적 "sometimes pollutes the beginning of the free list with small objects"

### 3. 할당 실패

```text
===== 3. 할당 실패 =====
my_malloc(4096) -> NULL (실패)
```

- 힙이 4096바이트인데 4096 요청 → 헤더 8바이트를 더하면 4104 필요 → **`NULL` 반환**
- 원서 17.2 "Growing The Heap" — 가장 단순한 접근은 **그냥 실패**. 실제 할당기는 `sbrk` 로 힙을 늘림

## 핵심 코드 읽기

### 헤더 역산 (원서 17.2)

```c
void my_free(void *ptr) {
    header_t *h = (header_t *) ptr - 1;   // 포인터 산술로 헤더 위치 역산
    assert(h->magic == MAGIC);            // 무결성 검사
    ...
```

- `(header_t *) ptr - 1` — `ptr` 을 `header_t *` 로 본 뒤 **1칸(8바이트) 뒤로**. 배열 인덱싱과 같은 규칙
- `magic` 검사가 **잘못된 `free` 를 잡는 장치** — 실습 4의 `09-invalid-free` 가 이 검사에 걸림

### 요청 크기에 헤더를 더하는 이유 (원서 17.2)

```c
int need = size + (int) sizeof(header_t);
```

- 원서 문장 — "when a user requests N bytes of memory, the library does not search for a free chunk of size N; rather, it searches for a free chunk of size N plus the size of the header"

### 분할 판단

```c
int remain = chunk->size - need;

if (remain >= (int) sizeof(node_t)) {
    /* 분할 — 남은 부분이 노드를 담을 만큼 크면 프리 리스트에 유지 */
    ...
} else {
    /* 남는 자리가 너무 작으면 청크 전체를 소비 (내부 단편화 발생) */
    ...
}
```

- 남는 공간이 **`node_t` 보다 작으면 리스트에 넣을 수 없음** — 노드 자체를 그 안에 놓아야 하므로
- 이때 버려지는 바이트가 **내부 단편화**. best fit 데모의 2바이트가 그 사례

### 병합

```c
/* 주소 순서를 유지하며 삽입 — 병합을 쉽게 하려는 목적 */
node_t *cur = head, *prev = NULL;
while (cur && cur < blk) { prev = cur; cur = cur->next; }
...
/* 뒤쪽 이웃과 인접하면 합친다 */
if (blk->next && (char *) blk + blk->size == (char *) blk->next) { ... }
/* 앞쪽 이웃과 인접하면 합친다 */
if (prev && (char *) prev + prev->size == (char *) blk) { ... }
```

- **주소 순 정렬 유지**가 병합의 전제 — 원서 17.3 first fit 논의의 "address-based ordering; by keeping the list ordered by the address of the free space, coalescing becomes easier"
- 인접 판정 — **한 청크의 시작 주소 + 크기 == 다음 청크의 시작 주소**

## 함정

| 증상 | 원인 | 대응 |
|---|---|---|
| 초기 프리 청크가 원서의 4088 이 아니라 4080 | arm64 는 `node_t` 가 정렬 패딩으로 **16바이트** (원서는 32비트 8바이트 가정) | 정상. `sizeof(node_t)` 를 출력해 확인 |
| 가운데 블록만 해제했는데 병합 안 됨 | **이웃이 사용 중**이면 병합 대상 없음 | 정상 동작. 원서 Figure 17.6 과 같은 상황 |
| best fit 이 프리 리스트 항목을 없앰 | 남는 공간(2바이트)이 `node_t`(16바이트)보다 작아 **청크 전체 소비** | 내부 단편화. 회수 불가 |
| `assert(h->magic == MAGIC)` 실패 | `my_malloc` 이 주지 않은 포인터를 `my_free` 에 넘김, 또는 헤더가 덮어써짐 | 버퍼 오버플로가 헤더를 훼손했을 가능성 확인 |
| 실제 `malloc` 과 주소가 전혀 다름 | 이 구현은 **자체 `mmap` 영역**을 관리. 시스템 `malloc` 과 무관 | 정상. 두 할당기가 독립적으로 공존 |
| `mmap` 이 `MAP_FAILED` 반환 | 매핑 실패 (자원 한계 등) | `assert` 로 즉시 중단하도록 구현되어 있음 |

## 확인 문제

1. `demo_split_coalesce()` 에서 **가운데 블록만 해제**했을 때 병합이 일어나지 않는 이유를 이웃 청크 상태로 설명할 것
2. `HEAP_SIZE` 를 8192 로 늘리면 초기 프리 청크 크기는 얼마가 되는가. 계산하고 실행으로 확인할 것
3. 분할 조건 `remain >= sizeof(node_t)` 를 `remain > 0` 으로 바꾸면 어떤 문제가 생기는가. 실제로 시도할 것
4. 병합 코드에서 **앞쪽 이웃 검사를 제거**하면 `demo_split_coalesce()` 결과가 어떻게 달라지는가. 원서 Figure 17.7(비병합 리스트)과 비교할 것
5. `next fit` 정책(마지막 탐색 위치를 기억)을 추가 구현하고 first fit 과 결과를 비교할 것
6. 헤더에서 `magic` 을 제거하면 무엇을 검출할 수 없게 되는가. 실습 4의 `09-invalid-free.c` 와 연결해 답할 것

## 관련 문서

- [[OS/ostep/docs/01-virtualization/17-free-space-management|17. 프리 공간 관리]] — 본 실습의 이론 배경
- [[OS/ostep/docs/01-virtualization/16-segmentation|16. 세그멘테이션]] — 외부 단편화가 발생하는 맥락
- [[OS/ostep/projects/04-memory-api/README|실습 4. 메모리 API와 오류 검출]] — 헤더 훼손·잘못된 `free` 의 실제 사례
- [[OS/ostep/projects/README|실습 프로젝트 인덱스]] — 전체 실습 커리큘럼
- [[C/docs/02-memory/heap-and-free|힙과 free]] — 실제 할당자의 동작
