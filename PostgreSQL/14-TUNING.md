---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - tuning
  - vacuum
  - parameter
  - monitoring
  - bulk-operation
  - status/verified
aliases:
  - DB 튜닝
  - VACUUM 튜닝
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]]

# 14. DB 튜닝 방법론

11번 문서가 "EXPLAIN을 읽고 인덱스를 거는 법"이라면, 이 문서는 **어디부터 손댈지 결정하는 절차**다.

> 이 문서의 수치(파라미터 권장값 등)는 일반적인 출발점이며, 실제 값은 반드시
> 해당 환경의 워크로드로 측정해 결정해야 한다.

---

## 1. 튜닝의 순서 — 이 순서를 지켜야 한다

효과 대비 비용 순이다. 위를 건너뛰고 아래로 가면 헛수고가 된다.

```
0. 측정 (무엇이 느린지 데이터로 확정)
   ↓
1. 쿼리/인덱스        ← 효과 90%. 여기서 대부분 끝난다
   ↓
2. 스키마/데이터 모델  ← 효과 크지만 변경 비용 높음
   ↓
3. 서버 파라미터       ← 효과 제한적. 잘못 만지면 악화
   ↓
4. 하드웨어/아키텍처   ← 돈으로 해결. 마지막 수단
```

**흔한 실수:** 느리다고 바로 `shared_buffers`부터 올린다. 인덱스 하나 없는
풀스캔 쿼리는 메모리를 아무리 늘려도 느리다.

---

## 2. 0단계 — 측정: 무엇이 문제인가

### 2-1. 병목 유형 판별

먼저 CPU 바운드인지, I/O 바운드인지, 락 대기인지를 가른다.

```sql
-- 현재 대기 이벤트 분포 (PG10+)
SELECT wait_event_type, wait_event, count(*)
FROM pg_stat_activity
WHERE state = 'active' AND pid <> pg_backend_pid()
GROUP BY 1,2 ORDER BY 3 DESC;
```

| `wait_event_type` | 의미 | 볼 곳 |
|---|---|---|
| `NULL` (대기 없음) | CPU 사용 중 | 쿼리 튜닝 (3장) |
| `IO` | 디스크 읽기/쓰기 | 인덱스, `shared_buffers`, 스토리지 |
| `Lock` | 행/테이블 락 대기 | 트랜잭션 설계 (6장) |
| `LWLock` | 내부 경합 | 접속 수, 체크포인트 |
| `Client` | 클라이언트 대기 | 애플리케이션 (N+1, 유휴 트랜잭션) |

```sql
-- OS 레벨 (별도 터미널)
-- top / vmstat 1 / iostat -x 1
```

### 2-2. 느린 쿼리 목록 확보

```sql
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
-- postgresql.conf: shared_preload_libraries = 'pg_stat_statements'  (재시작 필요)
--                  pg_stat_statements.track = all
```

**총 소요 시간(total_exec_time) 순으로 본다.** 평균이 느린 쿼리보다,
자주 호출되는 중간 속도 쿼리가 서버를 더 많이 잡아먹는 경우가 흔하다.

```sql
SELECT
    round(total_exec_time::numeric)                      AS total_ms,
    calls,
    round(mean_exec_time::numeric, 2)                    AS mean_ms,
    round(100 * total_exec_time / sum(total_exec_time) OVER (), 1) AS pct,
    rows / GREATEST(calls, 1)                            AS rows_per_call,
    left(regexp_replace(query, '\s+', ' ', 'g'), 90)     AS query
FROM pg_stat_statements
WHERE query NOT LIKE '%pg_stat_statements%'
ORDER BY total_exec_time DESC
LIMIT 20;
```

```sql
-- I/O를 많이 유발하는 쿼리 (캐시 미스)
SELECT calls, shared_blks_read, shared_blks_hit,
       round(100.0*shared_blks_hit/NULLIF(shared_blks_hit+shared_blks_read,0),1) AS hit_pct,
       left(query, 80) AS query
FROM pg_stat_statements
ORDER BY shared_blks_read DESC LIMIT 20;

-- 임시 파일을 쓰는 쿼리 (work_mem 부족)
SELECT calls, temp_blks_written, left(query,80)
FROM pg_stat_statements
WHERE temp_blks_written > 0
ORDER BY temp_blks_written DESC LIMIT 20;

-- 튜닝 전후 비교를 위해 리셋
SELECT pg_stat_statements_reset();
```

### 2-3. 로그 기반 수집

```sql
-- 느린 쿼리 로깅
ALTER SYSTEM SET log_min_duration_statement = '1s';
ALTER SYSTEM SET log_lock_waits = on;              -- 락 대기 기록
ALTER SYSTEM SET log_temp_files = 0;               -- 임시 파일 전부 기록
ALTER SYSTEM SET log_checkpoints = on;
ALTER SYSTEM SET log_autovacuum_min_duration = '1s';
ALTER SYSTEM SET log_line_prefix = '%m [%p] %u@%d app=%a ';
SELECT pg_reload_conf();
```

`auto_explain`으로 느린 쿼리의 실제 플랜까지 남기면 사후 분석이 쉬워진다.
```sql
-- shared_preload_libraries = 'pg_stat_statements,auto_explain'
ALTER SYSTEM SET auto_explain.log_min_duration = '3s';
ALTER SYSTEM SET auto_explain.log_analyze = on;
ALTER SYSTEM SET auto_explain.log_buffers = on;
ALTER SYSTEM SET auto_explain.log_nested_statements = on;   -- 함수 내부 쿼리까지
```
**주의: `log_analyze = on`은 모든 쿼리에 계측 오버헤드를 준다.** 운영에서는
`auto_explain.sample_rate`를 낮추거나 임시로만 켠다.

### 2-4. 목표 수치 정하기

측정 전에 목표를 정한다. "빠르게"는 목표가 아니다.

```
예) 주문 조회 API p95 응답 500ms 이하
    야간 배치 2시간 이내 완료
    피크 시 TPS 3000 유지
```

---

## 3. 1단계 — 쿼리/인덱스 튜닝 (효과 90%)

### 3-1. 진단 표준 명령

```sql
EXPLAIN (ANALYZE, BUFFERS, SETTINGS, VERBOSE) <쿼리>;
```
`BUFFERS` 없이 보는 플랜은 반쪽이다. I/O 양을 봐야 원인이 보인다.

### 3-2. 플랜에서 찾을 4가지 신호

| 신호 | 플랜에 나타나는 모습 | 조치 |
|---|---|---|
| **추정 오차** | `rows=100` vs `actual rows=100000` | `ANALYZE`, `SET STATISTICS`, 확장 통계 |
| **불필요한 행 읽기** | `Rows Removed by Filter: 950000` | 인덱스 추가/변경, 조건 재작성 |
| **디스크 정렬** | `Sort Method: external merge Disk: 24MB` | `work_mem` 상향, 인덱스로 정렬 제거 |
| **반복 과다** | `loops=50000` (Nested Loop 안쪽) | 조인 방식 유도, 인덱스 추가 |

```sql
-- 추정 오차가 클 때
ANALYZE t;
ALTER TABLE t ALTER COLUMN c SET STATISTICS 500;   -- 기본 100 → 500
ANALYZE t;

-- 컬럼 간 상관관계 (city와 zip처럼 종속적일 때)
CREATE STATISTICS st_t_city_zip (dependencies, ndistinct, mcv) ON city, zip FROM t;
ANALYZE t;
```

### 3-3. 인덱스 설계 3원칙

**① 등호 → 범위 → 정렬 순서로 컬럼 배치**
```sql
SELECT * FROM orders
WHERE status = 'pending' AND created_at >= '2026-01-01'
ORDER BY created_at DESC LIMIT 20;

CREATE INDEX idx_orders_status_created ON orders (status, created_at DESC);
-- status(등호) → created_at(범위+정렬). Sort 노드가 사라진다
```

**② 대상이 소수면 부분 인덱스**
```sql
-- 전체 1000만 건 중 pending은 1만 건
CREATE INDEX idx_orders_pending ON orders (created_at)
WHERE status = 'pending';
-- 인덱스 크기 1/1000, 갱신 비용도 그만큼 감소
```

**③ Index Only Scan을 노린다**
```sql
CREATE INDEX idx_x ON orders (user_id) INCLUDE (order_no, amount);
-- 플랜에 Index Only Scan + Heap Fetches: 0 이면 성공
-- Heap Fetches가 크면 VACUUM 부족 (visibility map 미갱신)
```

### 3-4. 인덱스 다이어트

인덱스는 쓰기 비용이다. INSERT 1건에 인덱스 10개면 11번 쓴다.

```sql
-- 안 쓰는 인덱스 (유니크/PK 제외)
SELECT s.schemaname, s.relname, s.indexrelname, s.idx_scan,
       pg_size_pretty(pg_relation_size(s.indexrelid)) AS size
FROM pg_stat_user_indexes s
JOIN pg_index i ON i.indexrelid = s.indexrelid
WHERE s.idx_scan = 0 AND NOT i.indisunique AND NOT i.indisprimary
ORDER BY pg_relation_size(s.indexrelid) DESC;
```
**판단 전에 통계 수집 기간을 확인한다.** 월배치에서만 쓰는 인덱스일 수 있다.
```sql
SELECT stats_reset FROM pg_stat_database WHERE datname = current_database();
```

```sql
-- 중복 인덱스: (a) 는 (a,b) 에 포함됨
SELECT indrelid::regclass AS tbl, array_agg(indexrelid::regclass) AS dup
FROM pg_index
GROUP BY indrelid, indkey, indpred, indclass
HAVING count(*) > 1;
```

### 3-5. 쿼리 재작성 패턴

```sql
-- 존재 확인: count 대신 EXISTS (첫 행에서 중단)
SELECT EXISTS(SELECT 1 FROM t WHERE cond);

-- 전체 건수 근사치 (정확한 count(*)는 항상 풀스캔)
SELECT reltuples::bigint FROM pg_class WHERE oid = 't'::regclass;

-- 깊은 페이지네이션: OFFSET → keyset
WHERE (created_at, id) < ($1, $2) ORDER BY created_at DESC, id DESC LIMIT 20;

-- 긴 IN 리스트 → 배열 바인딩 (플랜 캐시 재사용)
WHERE id = ANY($1::bigint[]);

-- OR → UNION ALL (각각 인덱스 사용)
SELECT * FROM t WHERE a = 1
UNION ALL
SELECT * FROM t WHERE b = 2 AND a IS DISTINCT FROM 1;

-- 그룹별 최신 1건: 윈도우 함수보다 DISTINCT ON / LATERAL
SELECT DISTINCT ON (user_id) * FROM orders ORDER BY user_id, created_at DESC;

-- 상관 서브쿼리 → LATERAL 또는 조인
-- N+1 루프 → 단일 조인 쿼리
-- 행 단위 UPDATE 루프 → UPDATE ... FROM 한 방
```

### 3-6. 인덱스를 못 쓰는 원인 (재확인)

11번 문서 2장 참조. 요약:
컬럼에 함수 적용 / 타입 불일치 / 선두 `%` / 복합 인덱스 좌측 미사용 /
낮은 선택도 / 통계 노후.

---

## 4. 2단계 — 스키마·데이터 모델 튜닝

### 4-1. 컬럼 정렬 (Column Tetris)

PostgreSQL은 컬럼을 정렬 요구에 맞춰 패딩한다. 순서만 바꿔도 테이블이 작아진다.

```sql
-- 나쁨: bool(1) → padding(7) → bigint(8) → int(4) → padding(4)
CREATE TABLE t (flag boolean, id bigint, cnt int, flag2 boolean);

-- 좋음: 큰 타입부터 (8바이트 → 4 → 2 → 1)
CREATE TABLE t (id bigint, cnt int, flag boolean, flag2 boolean);
```

```sql
-- 낭비 확인
SELECT a.attname, t.typname, t.typalign, t.typlen
FROM pg_attribute a JOIN pg_type t ON t.oid = a.atttypid
WHERE a.attrelid = 'mytable'::regclass AND a.attnum > 0
ORDER BY a.attnum;
```
효과는 보통 5~20%. 신규 설계 때 적용하고, 기존 테이블은 재작성 비용 때문에
대개 우선순위가 낮다.

### 4-2. 타입 선택

```sql
-- 과도한 타입 축소는 정렬 패딩 때문에 이득이 없을 수 있다
-- 그러나 명백한 낭비는 고친다
uuid  (16B)  vs  text로 저장한 UUID (37B)
timestamptz (8B)  vs  text 날짜
numeric (가변, 연산 느림)  vs  bigint (금액을 최소단위 정수로)
```

**대량 집계 테이블에서 `numeric` 연산은 `bigint`보다 눈에 띄게 느리다.**
원 단위처럼 소수가 없는 금액은 `bigint` 저장을 검토한다.

### 4-3. 정규화 vs 반정규화

조인 비용이 실측으로 병목일 때만 반정규화한다.
```sql
-- 반정규화 예: 집계 컬럼 유지
ALTER TABLE users ADD COLUMN order_count int NOT NULL DEFAULT 0;
-- 트리거 또는 배치로 갱신 → 정합성 관리 비용이 생긴다

-- 대안: 구체화 뷰 (읽기 지연 허용 시)
CREATE MATERIALIZED VIEW mv_user_stat AS
SELECT user_id, count(*) cnt, sum(amount) total FROM orders GROUP BY user_id;
CREATE UNIQUE INDEX ON mv_user_stat (user_id);
REFRESH MATERIALIZED VIEW CONCURRENTLY mv_user_stat;
```

### 4-4. 파티셔닝

**파티셔닝은 만능이 아니다.** 적용 기준:
- 시계열 데이터이고 **오래된 데이터를 통째로 삭제**해야 함 → `DETACH`가 즉시 끝남
- 쿼리 대부분이 파티션 키로 필터링됨 → partition pruning
- 단일 테이블이 수억 건 이상이고 VACUUM/인덱스 유지가 버거움

```sql
CREATE TABLE logs (id bigint, created_at timestamptz NOT NULL, msg text)
PARTITION BY RANGE (created_at);

CREATE TABLE logs_2026_08 PARTITION OF logs
    FOR VALUES FROM ('2026-08-01') TO ('2026-09-01');

-- pruning 확인
EXPLAIN SELECT * FROM logs WHERE created_at >= '2026-08-15';
-- 다른 파티션이 플랜에 안 나와야 성공

SET enable_partition_pruning = on;    -- 기본 on
SET enable_partitionwise_join = on;   -- 기본 off. 파티션끼리 조인
SET enable_partitionwise_aggregate = on;
```

**역효과:** 파티션 수가 수백 개를 넘으면 플래닝 시간이 늘어난다.
파티션 키가 안 들어간 쿼리는 전체 파티션을 스캔한다.

### 4-5. 파티션 자동 관리

```sql
-- 월별 파티션 생성 함수 예시
CREATE OR REPLACE PROCEDURE sp_create_next_partition(p_months int DEFAULT 3)
LANGUAGE plpgsql AS $$
DECLARE
    v_start date;
    v_name  text;
BEGIN
    FOR i IN 0..p_months LOOP
        v_start := date_trunc('month', current_date)::date + (i || ' month')::interval;
        v_name  := format('logs_%s', to_char(v_start, 'YYYY_MM'));
        IF to_regclass('public.' || v_name) IS NULL THEN
            EXECUTE format(
                'CREATE TABLE %I PARTITION OF logs FOR VALUES FROM (%L) TO (%L)',
                v_name, v_start, v_start + interval '1 month');
            RAISE NOTICE '생성: %', v_name;
        END IF;
    END LOOP;
END $$;
```

---

## 5. 3단계 — 서버 파라미터 튜닝

### 5-1. 메모리

| 파라미터 | 출발점 | 설명 |
|---|---|---|
| `shared_buffers` | RAM의 25% | 크게 잡는다고 계속 좋아지지 않음. 8~16GB 이상은 효과 체감 |
| `effective_cache_size` | RAM의 50~75% | **할당이 아니라 플래너 힌트.** OS 캐시 포함 추정치 |
| `work_mem` | 4~64MB | **연산 단위 할당.** 아래 계산식 참조 |
| `maintenance_work_mem` | 512MB~2GB | 인덱스 생성/VACUUM용 |
| `temp_buffers` | 8MB | 임시 테이블용 |

**`work_mem` 계산 함정:** 쿼리 하나가 정렬/해시 노드 여러 개를 가지면
**노드마다** `work_mem`을 쓴다. 병렬 워커까지 곱해진다.

```
최악 사용량 ≈ max_connections × 쿼리당 노드 수 × work_mem × (1 + 병렬 워커 수)
```

```sql
-- 전역은 보수적으로, 무거운 쿼리에서만 세션 단위로 올린다
SET work_mem = '256MB';
SELECT ... ;   -- 대형 집계
RESET work_mem;

-- 또는 특정 사용자/작업에만
ALTER ROLE batch_user SET work_mem = '512MB';
```

**`effective_cache_size`를 낮게 두면 플래너가 인덱스 스캔을 기피한다.**
실제 메모리를 잡아먹지 않으므로 넉넉히 잡는다.

### 5-2. 디스크/플래너 비용

| 파라미터 | HDD | SSD/NVMe |
|---|---|---|
| `random_page_cost` | 4.0 (기본) | **1.1** |
| `seq_page_cost` | 1.0 | 1.0 |
| `effective_io_concurrency` | 1~2 | **200** |

```sql
ALTER SYSTEM SET random_page_cost = 1.1;
ALTER SYSTEM SET effective_io_concurrency = 200;
SELECT pg_reload_conf();
```

**SSD 환경에서 `random_page_cost = 4.0` 방치는 가장 흔한 미세팅이다.**
인덱스 스캔 비용이 과대평가되어 불필요한 Seq Scan이 선택된다. 단일 변경으로
효과가 가장 큰 항목인 경우가 많으므로 우선 검토한다 (변경 후 주요 쿼리 플랜 재확인 필수).

### 5-3. WAL / 체크포인트

쓰기가 많은 시스템에서 중요하다.

| 파라미터 | 출발점 | 설명 |
|---|---|---|
| `max_wal_size` | 4~16GB | 작으면 체크포인트가 잦아져 I/O 스파이크 |
| `min_wal_size` | 1~2GB | |
| `checkpoint_timeout` | 15min | 기본 5min은 짧은 편 |
| `checkpoint_completion_target` | 0.9 (PG14+ 기본) | 체크포인트 I/O 분산 |
| `wal_compression` | on | CPU와 교환해 WAL 감소 |
| `wal_buffers` | 16MB | |
| `synchronous_commit` | on | off로 하면 빨라지나 **크래시 시 최근 커밋 소실** |

```sql
-- 체크포인트가 너무 잦은지 확인
SELECT * FROM pg_stat_checkpointer;   -- PG17+
SELECT * FROM pg_stat_bgwriter;       -- PG16 이하
-- checkpoints_req (요청 기반) 가 checkpoints_timed 보다 많으면 max_wal_size 부족
```

`synchronous_commit = off`는 **데이터 유실을 감수하는 설정**이다.
로그/분석 테이블처럼 유실 허용 가능한 트랜잭션에만 세션 단위로 적용한다.
```sql
SET LOCAL synchronous_commit = off;   -- 이 트랜잭션만
```

### 5-4. 병렬 처리

```sql
max_worker_processes = CPU 코어 수
max_parallel_workers = CPU 코어 수
max_parallel_workers_per_gather = 2~4     -- 쿼리 하나가 쓸 워커
max_parallel_maintenance_workers = 4      -- 인덱스 생성 가속

-- 병렬이 안 걸릴 때 임계값 조정
SET min_parallel_table_scan_size = '8MB';  -- 기본 8MB
SET parallel_setup_cost = 100;             -- 기본 1000. 낮추면 병렬 선호
```
**OLTP에서 `max_parallel_workers_per_gather`를 크게 잡으면 역효과다.**
짧은 쿼리에 워커 기동 비용만 든다. 분석 워크로드에서 올린다.

### 5-5. 접속 관리

```sql
max_connections = 100~200   -- 크게 잡지 말 것
```

**PostgreSQL은 접속마다 프로세스를 만든다.** 수천 접속은 컨텍스트 스위칭과
메모리로 서버를 죽인다. **PgBouncer 같은 커넥션 풀러가 사실상 필수다.**

```
애플리케이션 풀 (수백) → PgBouncer (transaction pooling) → PostgreSQL (수십)
```

```sql
-- 접속 현황
SELECT state, count(*) FROM pg_stat_activity GROUP BY state;
-- idle in transaction 이 많으면 애플리케이션 트랜잭션 관리 문제
```

```sql
-- 유휴 트랜잭션 방치 차단
ALTER SYSTEM SET idle_in_transaction_session_timeout = '5min';
ALTER SYSTEM SET statement_timeout = '30s';       -- 애플리케이션 롤 단위 권장
ALTER SYSTEM SET lock_timeout = '3s';
```

### 5-6. 설정 적용 방법

```sql
ALTER SYSTEM SET work_mem = '32MB';    -- postgresql.auto.conf 에 기록
SELECT pg_reload_conf();               -- reload 가능한 항목 즉시 반영

-- 재시작 필요 여부 확인
SELECT name, setting, pending_restart FROM pg_settings WHERE pending_restart;

-- 현재 값과 출처
SELECT name, setting, unit, source, sourcefile
FROM pg_settings WHERE name IN ('work_mem','shared_buffers','random_page_cost');

ALTER SYSTEM RESET work_mem;
```

**적용 범위 우선순위 (좁은 쪽이 이긴다):**
`SET LOCAL` (트랜잭션) > `SET` (세션) > `ALTER ROLE`/`ALTER DATABASE` > `ALTER SYSTEM` > `postgresql.conf`

---

## 6. 트랜잭션·동시성 튜닝

### 6-1. 락 경합 진단

```sql
-- 누가 누구를 막고 있는가
SELECT
    blocked.pid          AS blocked_pid,
    blocked.usename      AS blocked_user,
    now() - blocked.query_start AS blocked_dur,
    left(blocked.query, 60)     AS blocked_query,
    blocking.pid         AS blocking_pid,
    left(blocking.query, 60)    AS blocking_query,
    blocking.state       AS blocking_state
FROM pg_stat_activity blocked
JOIN pg_stat_activity blocking
  ON blocking.pid = ANY(pg_blocking_pids(blocked.pid))
WHERE cardinality(pg_blocking_pids(blocked.pid)) > 0;

-- 락 대기 상세
SELECT locktype, relation::regclass, mode, granted, pid
FROM pg_locks WHERE NOT granted;
```

### 6-2. 경합 줄이기

```sql
-- ① 트랜잭션을 짧게. 사용자 입력/외부 API 호출을 트랜잭션 안에 넣지 않는다

-- ② 락 순서 통일 (데드락 예방)
SELECT * FROM accounts WHERE id = ANY($1) ORDER BY id FOR UPDATE;

-- ③ 핫 로우 분산: 단일 카운터 → 샤딩
CREATE TABLE counter (shard smallint, cnt bigint, PRIMARY KEY (shard));
UPDATE counter SET cnt = cnt + 1 WHERE shard = (random()*16)::int;
SELECT sum(cnt) FROM counter;   -- 조회 시 합산

-- ④ 큐는 SKIP LOCKED
SELECT id FROM jobs WHERE status='pending'
ORDER BY created_at LIMIT 10 FOR UPDATE SKIP LOCKED;

-- ⑤ 낙관적 잠금으로 대기 제거
UPDATE t SET v = $1, version = version + 1 WHERE id = $2 AND version = $3;
-- 영향 행 0 → 재시도
```

### 6-3. HOT Update 유지

인덱스 컬럼을 갱신하지 않으면 PostgreSQL은 **HOT(Heap-Only Tuple) 업데이트**를 써서
인덱스 갱신을 생략한다. 훨씬 싸다.

```sql
-- HOT 비율 확인 (높을수록 좋음)
SELECT relname, n_tup_upd, n_tup_hot_upd,
       round(100.0 * n_tup_hot_upd / NULLIF(n_tup_upd,0), 1) AS hot_pct
FROM pg_stat_user_tables WHERE n_tup_upd > 0 ORDER BY n_tup_upd DESC;
```

HOT 비율이 낮다면:
- 자주 갱신되는 컬럼에 걸린 인덱스를 제거할 수 있는지 검토
- `fillfactor`를 낮춰 같은 페이지에 새 버전을 둘 공간 확보

```sql
ALTER TABLE t SET (fillfactor = 80);   -- 기본 100
VACUUM FULL t;                          -- 적용하려면 재작성 필요 (락 주의)
```

---

## 7. 유지보수 튜닝 — VACUUM / 통계

MVCC 특성상 **VACUUM 관리가 PostgreSQL 튜닝의 절반**이다.

### 7-1. 팽창(bloat) 진단

```sql
SELECT relname,
       n_live_tup, n_dead_tup,
       round(100.0 * n_dead_tup / NULLIF(n_live_tup + n_dead_tup, 0), 1) AS dead_pct,
       last_vacuum, last_autovacuum, last_analyze, last_autoanalyze,
       autovacuum_count
FROM pg_stat_user_tables
WHERE n_dead_tup > 10000
ORDER BY n_dead_tup DESC LIMIT 20;
```
`dead_pct`가 20%를 넘고 `last_autovacuum`이 오래됐다면 autovacuum이 따라가지 못하는 것이다.

### 7-2. autovacuum이 못 따라가는 3가지 원인

**① 임계값이 대형 테이블에 부적합**
```
기본: autovacuum_vacuum_threshold(50) + scale_factor(0.2) × 총 행수
→ 1억 건 테이블은 2천만 건이 죽어야 시작한다
```
```sql
-- 대형/고빈도 갱신 테이블은 개별 설정
ALTER TABLE big_table SET (
    autovacuum_vacuum_scale_factor  = 0.01,   -- 1%
    autovacuum_vacuum_threshold     = 1000,
    autovacuum_analyze_scale_factor = 0.005,
    autovacuum_vacuum_cost_delay    = 0       -- 지연 없이 (I/O 여유 있을 때)
);
```

**② autovacuum이 너무 느림 (I/O 스로틀)**
```sql
ALTER SYSTEM SET autovacuum_max_workers = 5;
ALTER SYSTEM SET autovacuum_vacuum_cost_limit = 2000;   -- 기본 200
ALTER SYSTEM SET autovacuum_naptime = '30s';            -- 기본 1min
ALTER SYSTEM SET maintenance_work_mem = '1GB';
```

**③ 긴 트랜잭션이 회수를 막음**
```sql
-- 가장 오래된 트랜잭션
SELECT pid, now() - xact_start AS age, state, left(query,60)
FROM pg_stat_activity WHERE xact_start IS NOT NULL
ORDER BY xact_start LIMIT 5;

-- 방치된 replication slot / prepared transaction 도 동일하게 막는다
SELECT slot_name, active, wal_status FROM pg_replication_slots;
SELECT * FROM pg_prepared_xacts;
```

### 7-3. 트랜잭션 ID 소진 감시

방치하면 **DB가 읽기 전용으로 강제 전환된다.** 반드시 모니터링한다.

```sql
SELECT datname,
       age(datfrozenxid) AS xid_age,
       round(100.0 * age(datfrozenxid) / 2147483648, 1) AS pct_to_wraparound
FROM pg_database ORDER BY xid_age DESC;
-- autovacuum_freeze_max_age(기본 2억) 초과 시 강제 vacuum 발동
-- 20억 접근 시 위험
```

### 7-4. 팽창 해소

```sql
VACUUM (ANALYZE, VERBOSE) t;      -- 일상적 회수 (공간은 OS 반환 안 됨)
VACUUM FULL t;                     -- 재작성. ACCESS EXCLUSIVE 락 — 운영 중 금지

-- 운영 중 무중단 재작성은 pg_repack 확장 사용
-- pg_repack -t mytable -d mydb

REINDEX INDEX CONCURRENTLY idx_x;  -- 인덱스 팽창 해소 (PG12+)
```

### 7-5. 정기 점검 항목

| 주기 | 항목 |
|---|---|
| 상시 | 대기 이벤트, 락 대기, 접속 수, 긴 트랜잭션 |
| 일 | 느린 쿼리 Top 20, dead tuple 비율, 캐시 히트율 |
| 주 | 미사용 인덱스, 테이블 증가 추이, 체크포인트 빈도 |
| 월 | XID age, 인덱스 팽창, 파티션 생성/삭제, 파라미터 재검토 |

---

## 8. 대량 작업 튜닝

### 8-1. 대량 적재

```sql
-- 1. COPY 사용 (INSERT 대비 수십 배)
COPY t FROM STDIN WITH (FORMAT csv);

-- 2. 인덱스/제약/트리거 제거 후 적재, 이후 재생성
DROP INDEX idx_a, idx_b;
ALTER TABLE t DISABLE TRIGGER ALL;
-- 적재
ALTER TABLE t ENABLE TRIGGER ALL;
CREATE INDEX CONCURRENTLY idx_a ON t (...);

-- 3. 세션 설정
SET maintenance_work_mem = '2GB';
SET max_parallel_maintenance_workers = 4;
SET synchronous_commit = off;          -- 유실 감수 가능할 때만

-- 4. UNLOGGED 스테이징 활용
CREATE UNLOGGED TABLE stg (LIKE t INCLUDING DEFAULTS);
COPY stg FROM ...;
INSERT INTO t SELECT * FROM stg;

-- 5. 마무리 필수
ANALYZE t;
```

### 8-2. 대량 UPDATE/DELETE

한 방에 처리하면 락 유지 시간, WAL 폭증, 복제 지연이 발생한다. **배치로 나눈다.**

```sql
-- 배치 삭제
DO $$
DECLARE v_cnt int;
BEGIN
    LOOP
        DELETE FROM logs
        WHERE id IN (SELECT id FROM logs WHERE created_at < '2025-01-01' LIMIT 10000);
        GET DIAGNOSTICS v_cnt = ROW_COUNT;
        EXIT WHEN v_cnt = 0;
        COMMIT;                 -- 프로시저/DO 블록(PG11+)에서 가능
        PERFORM pg_sleep(0.1);  -- 복제 지연 완화
    END LOOP;
END $$;
```

**파티션 테이블이라면 배치 삭제 대신 `DETACH`가 압도적으로 빠르다.**
```sql
ALTER TABLE logs DETACH PARTITION logs_2024_01 CONCURRENTLY;
DROP TABLE logs_2024_01;
```

---

## 9. 튜닝 시나리오별 처방

| 증상 | 우선 확인 | 처방 |
|---|---|---|
| 특정 쿼리만 느림 | `EXPLAIN (ANALYZE, BUFFERS)` | 인덱스, 쿼리 재작성 |
| 전체적으로 느려짐 | dead tuple, 캐시 히트율, 접속 수 | VACUUM, 팽창 해소, 풀러 도입 |
| 갑자기 느려짐 | 통계 노후, 플랜 변경, 락 | `ANALYZE`, 락 확인 |
| 피크 시간만 느림 | 접속 수, `work_mem` 총량, 체크포인트 | 풀러, 체크포인트 분산 |
| 쓰기가 느림 | 인덱스 개수, WAL, `synchronous_commit` | 인덱스 정리, `max_wal_size` |
| 디스크가 계속 증가 | bloat, XID age, WAL 적체 | autovacuum 튜닝, slot 정리 |
| 간헐적 타임아웃 | 락 대기, 체크포인트 I/O 스파이크 | `lock_timeout`, 체크포인트 조정 |
| 배치가 느림 | 임시 파일, 병렬도 | `work_mem`, 병렬 워커, COPY |
| 커넥션 부족 | `idle in transaction` | 애플리케이션 트랜잭션 범위 수정 |

---

## 10. 상시 모니터링 쿼리 모음

```sql
-- 캐시 히트율 (99% 미만이면 shared_buffers 검토)
SELECT round(100.0 * sum(heap_blks_hit) /
             NULLIF(sum(heap_blks_hit) + sum(heap_blks_read), 0), 2) AS cache_hit_pct
FROM pg_statio_user_tables;

-- 인덱스 사용률 (낮으면 인덱스 설계 점검)
SELECT relname,
       round(100.0 * idx_scan / NULLIF(seq_scan + idx_scan, 0), 1) AS idx_pct,
       seq_scan, idx_scan, n_live_tup
FROM pg_stat_user_tables
WHERE seq_scan + idx_scan > 0 AND n_live_tup > 10000
ORDER BY idx_pct NULLS FIRST LIMIT 20;

-- 테이블 크기 상위
SELECT relname,
       pg_size_pretty(pg_total_relation_size(relid))                      AS total,
       pg_size_pretty(pg_relation_size(relid))                            AS table_only,
       pg_size_pretty(pg_total_relation_size(relid) - pg_relation_size(relid)) AS idx_toast
FROM pg_stat_user_tables ORDER BY pg_total_relation_size(relid) DESC LIMIT 20;

-- 커밋/롤백 비율, 데드락, 임시 파일
SELECT datname, xact_commit, xact_rollback, deadlocks,
       temp_files, pg_size_pretty(temp_bytes) AS temp_size,
       blks_hit, blks_read
FROM pg_stat_database WHERE datname = current_database();

-- 복제 지연 (있는 경우)
SELECT client_addr, state, sent_lsn, replay_lsn,
       pg_wal_lsn_diff(sent_lsn, replay_lsn) AS lag_bytes
FROM pg_stat_replication;

-- 진행 중인 유지보수 작업 (PG9.6+)
SELECT * FROM pg_stat_progress_vacuum;
SELECT * FROM pg_stat_progress_create_index;
```

---

## 11. 튜닝 원칙

1. **측정 없이 바꾸지 않는다.** 추측 기반 튜닝은 대개 악화시킨다.
2. **한 번에 하나만 바꾼다.** 동시에 세 개 바꾸면 무엇이 효과였는지 모른다.
3. **변경 전후를 같은 기준으로 비교한다.** `pg_stat_statements_reset()` 후 동일 시간대 측정.
4. **쿼리부터, 파라미터는 나중에.** 90%는 쿼리와 인덱스에서 끝난다.
5. **운영 데이터 규모로 검증한다.** 1만 건에서 빠른 쿼리가 1천만 건에서 빠르다는 보장은 없다.
6. **롤백 계획을 먼저 세운다.** `ALTER SYSTEM RESET`, 인덱스 `DROP` 절차를 미리 준비.
7. **인덱스 추가는 공짜가 아니다.** 읽기 이득과 쓰기 비용을 함께 계산한다.
8. **변경 이력을 남긴다.** 무엇을, 왜, 어떤 수치 근거로 바꿨는지 기록.

## 관련 문서

- [[PostgreSQL/11-PERFORMANCE|11. 성능 — EXPLAIN, 인덱스 전략, 통계]] — 쿼리 단위 EXPLAIN·인덱스 문법
- [[PostgreSQL/10-TRANSACTION|10. 트랜잭션, 격리 수준, 락]] — MVCC·락과 VACUUM의 관계
- [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]] — 파티셔닝·인덱스 정의
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
