---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - performance
  - explain
  - index
  - planner
  - statistics
  - status/verified
aliases:
  - EXPLAIN
  - 인덱스 전략
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/10-TRANSACTION|10. 트랜잭션, 격리 수준, 락]]  ·  **다음** [[PostgreSQL/12-CONVENTIONS|12. 코딩 컨벤션과 안티패턴]]

# 11. 성능 — EXPLAIN, 인덱스 전략, 통계

## 1. EXPLAIN

```sql
EXPLAIN SELECT * FROM t WHERE id = 1;              -- 예상 플랜만 (실행 안 함)
EXPLAIN ANALYZE SELECT ...;                        -- 실제 실행 + 소요 시간
EXPLAIN (ANALYZE, BUFFERS) SELECT ...;             -- 버퍼 I/O 포함 (거의 필수)
EXPLAIN (ANALYZE, BUFFERS, VERBOSE, FORMAT JSON) SELECT ...;
EXPLAIN (ANALYZE, BUFFERS, SETTINGS, WAL) SELECT ...;
EXPLAIN (ANALYZE, SERIALIZE) SELECT ...;           -- PG17+ 결과 직렬화 비용 포함
```

**`EXPLAIN ANALYZE`는 쿼리를 실제로 실행한다.** UPDATE/DELETE에 쓸 때는 트랜잭션으로 감쌀 것.
```sql
BEGIN;
EXPLAIN ANALYZE DELETE FROM t WHERE ...;
ROLLBACK;
```

### 플랜 읽는 법
```
Seq Scan on t  (cost=0.00..18.50 rows=850 width=36)
                                 (actual time=0.012..0.245 rows=1000 loops=1)
```
- `cost=시작..전체` — 임의 단위. 절대값이 아닌 **상대 비교용**
- `rows` — 플래너 추정치
- `actual rows` — 실제 행 수
- `loops` — 반복 횟수. **실제 총 시간 = actual time × loops**

**핵심 진단: 추정 `rows`와 `actual rows`의 차이.** 10배 이상 벌어지면 통계 문제다.
```sql
ANALYZE t;                                       -- 통계 갱신
ALTER TABLE t ALTER COLUMN c SET STATISTICS 500; -- 히스토그램 정밀도 상향 (기본 100)
ANALYZE t;
```

### 주요 노드
| 노드 | 의미 | 개선 방향 |
|---|---|---|
| `Seq Scan` | 전체 스캔 | 소량 테이블이면 정상. 대량+선택도 높으면 인덱스 |
| `Index Scan` | 인덱스 → 테이블 접근 | 정상 |
| `Index Only Scan` | 인덱스만으로 해결 | 최적. `Heap Fetches` 크면 VACUUM 필요 |
| `Bitmap Heap Scan` | 인덱스로 블록 수집 후 일괄 읽기 | 중간 선택도에서 정상 |
| `Nested Loop` | 반복 조인 | 안쪽 loops가 크면 문제 |
| `Hash Join` | 해시 조인 | 대량 등호 조인에 적합 |
| `Merge Join` | 정렬 병합 | 정렬된 입력에 적합 |
| `Sort` | 정렬 | `Disk`로 나오면 `work_mem` 부족 |
| `Materialize` | 중간 결과 저장 | |
| `Memoize` (PG14+) | 반복 조회 캐시 | |

### 경고 신호
```
Rows Removed by Filter: 950000     ← 대부분 버림. 인덱스나 조건 재검토
Sort Method: external merge  Disk: 24MB   ← work_mem 부족
Heap Fetches: 500000               ← Index Only Scan인데 힙 접근 과다 → VACUUM
Buffers: shared read=100000        ← 캐시 미스 대량
loops=10000                        ← Nested Loop 안쪽 반복 과다
Filter: (col = 'x'::text)          ← 인덱스 못 씀 (Index Cond가 아님)
```

```sql
-- 세션 단위 조정
SET work_mem = '128MB';         -- 정렬/해시 메모리 (연산마다 할당됨. 주의)
SET max_parallel_workers_per_gather = 4;
RESET ALL;
```

### auto_explain
```sql
LOAD 'auto_explain';
SET auto_explain.log_min_duration = '1s';
SET auto_explain.log_analyze = on;
SET auto_explain.log_buffers = on;
-- 느린 쿼리의 플랜이 자동으로 로그에 남음
```

## 2. 인덱스가 안 쓰이는 흔한 원인

```sql
-- 1. 컬럼에 함수/연산 적용
WHERE upper(name) = 'KIM'              -- 안 됨
CREATE INDEX ON t (upper(name));       -- 표현식 인덱스로 해결

WHERE created_at::date = '2026-08-03'  -- 안 됨
WHERE created_at >= '2026-08-03' AND created_at < '2026-08-04'   -- 범위로

WHERE id + 1 = 10                      -- 안 됨
WHERE id = 9                           -- 이항

-- 2. 타입 불일치 (암묵 캐스팅)
WHERE varchar_col = 123                -- 캐스팅 발생 가능
WHERE bigint_col = '123'               -- 보통 OK (리터럴은 추론됨)
-- 파라미터 바인딩 타입을 컬럼과 맞출 것

-- 3. 선두 와일드카드
WHERE name LIKE '%kim%'                -- B-tree 안 됨
CREATE INDEX ON t USING gin (name gin_trgm_ops);   -- pg_trgm 으로 해결
WHERE name LIKE 'kim%'                 -- 이건 B-tree 사용 가능

-- 4. C 이외 로케일에서 LIKE 접두 검색
CREATE INDEX ON t (name text_pattern_ops);   -- LIKE 'x%' 전용 연산자 클래스

-- 5. OR 조건
WHERE a = 1 OR b = 2                   -- 각각 인덱스 있어도 비효율
-- → UNION 으로 분리하거나 BitmapOr 확인

-- 6. 복합 인덱스 좌측 미사용
CREATE INDEX ON t (a, b);
WHERE b = 1                            -- 사용 못 함(스킵 스캔 미지원)

-- 7. 선택도가 낮음 (전체의 상당수를 반환)
-- 이 경우 Seq Scan 이 실제로 더 빠르다. 정상 동작임

-- 8. 통계가 오래됨
ANALYZE t;
```

## 3. 인덱스 설계 원칙

```sql
-- 등호 조건 컬럼을 앞에, 범위 조건을 뒤에
WHERE status = 'a' AND created_at > '2026-01-01' ORDER BY created_at
CREATE INDEX ON t (status, created_at);    -- 이 순서

-- ORDER BY + LIMIT 은 인덱스 정렬 순서와 일치시키면 Sort 제거
SELECT * FROM t WHERE status='a' ORDER BY created_at DESC LIMIT 10;
CREATE INDEX ON t (status, created_at DESC);

-- 부분 인덱스: 대상이 소수일 때 크기/성능 모두 유리
CREATE INDEX ON orders (created_at) WHERE status = 'pending';

-- 커버링 인덱스: Index Only Scan 유도
CREATE INDEX ON t (a) INCLUDE (b, c);

-- 중복 인덱스 제거: (a) 는 (a,b) 에 포함됨
```

### 인덱스 비용
인덱스는 공짜가 아니다. 쓰기마다 갱신되고 디스크를 차지한다.
```sql
-- 사용되지 않는 인덱스
SELECT s.relname, s.indexrelname, s.idx_scan,
       pg_size_pretty(pg_relation_size(s.indexrelid)) AS size
FROM pg_stat_user_indexes s
JOIN pg_index i ON i.indexrelid = s.indexrelid
WHERE s.idx_scan = 0 AND NOT i.indisunique
ORDER BY pg_relation_size(s.indexrelid) DESC;
```

## 4. 통계와 플래너

```sql
-- 확장 통계 (컬럼 간 상관관계, PG10+)
CREATE STATISTICS st_city_zip (dependencies, ndistinct, mcv)
    ON city, zip FROM addr;
ANALYZE addr;
-- city와 zip이 강하게 연관될 때 추정 오차를 줄여준다

-- 통계 확인
SELECT attname, n_distinct, most_common_vals, correlation
FROM pg_stats WHERE tablename = 't';
```

`correlation`이 1 또는 -1에 가까우면 물리 순서와 값 순서가 일치 → Index Scan이 유리.
0에 가까우면 랜덤 I/O가 많아 Bitmap Scan이 선택된다.

```sql
-- 물리 순서 재정렬 (ACCESS EXCLUSIVE 락, 운영 중 주의)
CLUSTER t USING idx_created_at;
```

## 5. pg_stat_statements — 느린 쿼리 찾기

```sql
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
-- postgresql.conf: shared_preload_libraries = 'pg_stat_statements' (재시작 필요)

-- 총 소요 시간 상위
SELECT round(total_exec_time::numeric, 0) AS total_ms,
       calls,
       round(mean_exec_time::numeric, 2) AS mean_ms,
       rows,
       left(query, 100) AS query
FROM pg_stat_statements
ORDER BY total_exec_time DESC LIMIT 20;

-- 캐시 히트율 낮은 쿼리
SELECT query, shared_blks_hit, shared_blks_read,
       round(100.0*shared_blks_hit/NULLIF(shared_blks_hit+shared_blks_read,0), 1) AS hit_pct
FROM pg_stat_statements ORDER BY shared_blks_read DESC LIMIT 20;

SELECT pg_stat_statements_reset();
```

**평균이 느린 쿼리보다 `total_exec_time`이 큰 쿼리를 먼저 잡는다.**
10ms 쿼리가 100만 번 호출되는 것이 1초 쿼리 한 번보다 문제다.

## 6. 쿼리 작성 패턴

```sql
-- COUNT(*) 최적화: 정확한 전체 카운트는 항상 풀스캔
SELECT count(*) FROM big;                            -- 느림
SELECT reltuples::bigint FROM pg_class WHERE relname='big';   -- 근사치, 즉시
-- 페이지네이션 총건수는 근사치 + "약 N건" 표기가 실용적

-- EXISTS 가 count > 0 보다 빠름
SELECT EXISTS(SELECT 1 FROM t WHERE x);              -- 첫 행에서 중단
SELECT count(*) > 0 FROM t WHERE x;                  -- 전부 셈

-- IN (긴 리스트) → 배열 바인딩 또는 VALUES 조인
WHERE id = ANY($1::int[])
JOIN (VALUES (1),(2),(3)) v(id) ON t.id = v.id

-- OFFSET 대신 keyset 페이징
WHERE (created_at, id) < ($1, $2) ORDER BY created_at DESC, id DESC LIMIT 20

-- SELECT * 지양: 필요한 컬럼만 (Index Only Scan 가능성, 네트워크 절감)

-- N+1 제거: 루프 대신 조인 또는 배열 IN
-- 대량 INSERT: 다중 VALUES 또는 COPY
INSERT INTO t (a,b) VALUES (1,'x'),(2,'y'), ... ;    -- 수백~수천 건씩
COPY t FROM STDIN;                                    -- 최대 성능

-- 대량 UPDATE/DELETE는 배치 분할 (락 시간, WAL 폭증 방지)
DELETE FROM t WHERE id IN (SELECT id FROM t WHERE cond LIMIT 10000);
```

## 7. 대량 적재 튜닝

```sql
-- 1. 인덱스/제약 제거 → 적재 → 재생성
ALTER TABLE t DROP CONSTRAINT ...;
DROP INDEX ...;
COPY t FROM ...;
CREATE INDEX ...;
ALTER TABLE t ADD CONSTRAINT ...;

-- 2. 트리거 비활성화
ALTER TABLE t DISABLE TRIGGER ALL;
-- 적재
ALTER TABLE t ENABLE TRIGGER ALL;

-- 3. 세션 설정
SET synchronous_commit = off;       -- 커밋 지연 허용 (크래시 시 최근 커밋 소실 가능)
SET maintenance_work_mem = '1GB';   -- 인덱스 생성 가속

-- 4. UNLOGGED 테이블에 적재 후 전환
CREATE UNLOGGED TABLE staging (...);
COPY staging FROM ...;
ALTER TABLE staging SET LOGGED;

-- 5. 적재 후 반드시
ANALYZE t;
```

## 8. 주요 설정 파라미터

| 파라미터 | 권장 기준 |
|---|---|
| `shared_buffers` | RAM의 25% |
| `effective_cache_size` | RAM의 50~75% (플래너 힌트, 실제 할당 아님) |
| `work_mem` | 정렬/해시당 할당. 동시 접속 수 고려해 보수적으로 |
| `maintenance_work_mem` | 인덱스 생성/VACUUM용. 1GB 정도 |
| `random_page_cost` | SSD면 1.1 (기본 4.0은 HDD 기준) |
| `effective_io_concurrency` | SSD면 200 |
| `max_connections` | 크게 잡지 말 것. PgBouncer 등 풀러 사용 |

```sql
SHOW ALL;
SELECT name, setting, unit, source FROM pg_settings WHERE name LIKE '%work_mem%';
SELECT pg_reload_conf();    -- 재시작 불필요한 설정 반영
```

**`random_page_cost = 4.0` (기본값)은 SSD 환경에서 인덱스 스캔을 과소평가하게 만든다.**
SSD라면 1.1로 낮추는 것이 인덱스 사용률 개선에 가장 효과가 큰 단일 변경인 경우가 많다.
(운영 반영 전 실제 워크로드로 검증 필요)

## 9. 캐시 히트율 확인

```sql
SELECT sum(heap_blks_hit)*100.0/NULLIF(sum(heap_blks_hit)+sum(heap_blks_read),0) AS hit_pct
FROM pg_statio_user_tables;
-- 99% 미만이면 shared_buffers 검토
```

## 관련 문서

- [[PostgreSQL/14-TUNING|14. DB 튜닝 방법론]] — 시스템 전체 관점 튜닝 절차 — 어디부터 손댈지
- [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]] — 인덱스 생성 문법
- [[PostgreSQL/04-JOIN-SUBQUERY|04. JOIN, 서브쿼리, CTE]] — 조인 알고리즘과 실행 계획
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
