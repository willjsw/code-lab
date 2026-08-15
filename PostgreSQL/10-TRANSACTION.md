---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - transaction
  - isolation
  - lock
  - mvcc
  - savepoint
  - status/verified
aliases:
  - 격리 수준
  - MVCC
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/09-POSTGRES-ONLY|09. PostgreSQL 고유 기능]]  ·  **다음** [[PostgreSQL/11-PERFORMANCE|11. 성능 — EXPLAIN, 인덱스 전략, 통계]]

# 10. 트랜잭션, 격리 수준, 락

## 1. 기본 문법

```sql
BEGIN;                      -- 또는 START TRANSACTION;
    INSERT INTO t VALUES (1);
    UPDATE t SET a=1;
COMMIT;                     -- 또는 END;

BEGIN;
    ...
ROLLBACK;                   -- 또는 ABORT;

-- 옵션 지정
BEGIN ISOLATION LEVEL REPEATABLE READ READ WRITE;
BEGIN TRANSACTION READ ONLY;
BEGIN ISOLATION LEVEL SERIALIZABLE DEFERRABLE;   -- 읽기 전용 + 직렬화 대기
```

**PostgreSQL은 기본이 자동 커밋(autocommit)이다.** 명시적 `BEGIN` 없이 실행한
각 statement는 자체 트랜잭션으로 즉시 커밋된다.

## 2. SAVEPOINT

```sql
BEGIN;
    INSERT INTO t VALUES (1);
    SAVEPOINT sp1;
    INSERT INTO t VALUES (2);
    ROLLBACK TO SAVEPOINT sp1;   -- 2번만 취소, 1번은 유지
    INSERT INTO t VALUES (3);
    RELEASE SAVEPOINT sp1;
COMMIT;
```

**중요: PostgreSQL은 트랜잭션 안에서 에러가 나면 전체가 abort 상태가 된다.**
이후 모든 명령이 `current transaction is aborted` 에러로 거부된다.
Oracle처럼 에러 후 이어서 진행하려면 SAVEPOINT가 필수다.

```sql
BEGIN;
    INSERT INTO t VALUES (1);
    INSERT INTO t VALUES ('bad');   -- 에러
    INSERT INTO t VALUES (2);       -- ERROR: transaction is aborted
ROLLBACK;

-- 해결
BEGIN;
    INSERT INTO t VALUES (1);
    SAVEPOINT sp;
    INSERT INTO t VALUES ('bad');   -- 에러
    ROLLBACK TO sp;                 -- 여기서 복구
    INSERT INTO t VALUES (2);       -- OK
COMMIT;
```

psql에서는 `\set ON_ERROR_ROLLBACK on` 으로 statement마다 자동 SAVEPOINT를 걸 수 있다.

PL/pgSQL의 `EXCEPTION` 블록은 내부적으로 SAVEPOINT를 생성하므로 같은 효과를 낸다.

## 3. 격리 수준

```sql
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;      -- 기본
SET TRANSACTION ISOLATION LEVEL REPEATABLE READ;
SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;
SET default_transaction_isolation = 'repeatable read';   -- 세션 기본값
SHOW transaction_isolation;
```

| 수준 | Dirty Read | Non-repeatable Read | Phantom Read | 직렬화 이상 |
|---|---|---|---|---|
| READ UNCOMMITTED | 불가* | 가능 | 가능 | 가능 |
| **READ COMMITTED** (기본) | 불가 | 가능 | 가능 | 가능 |
| REPEATABLE READ | 불가 | 불가 | **불가** | 가능 |
| SERIALIZABLE | 불가 | 불가 | 불가 | 불가 |

\* PostgreSQL은 READ UNCOMMITTED를 요청해도 READ COMMITTED로 동작한다 (dirty read 없음).

**PostgreSQL의 REPEATABLE READ는 표준보다 강하다.** 스냅샷 격리(Snapshot Isolation)를
구현하므로 팬텀 리드도 발생하지 않는다.

### READ COMMITTED 동작
statement마다 새 스냅샷을 잡는다. 같은 트랜잭션 안에서 같은 쿼리가 다른 결과를 낼 수 있다.

```sql
-- 세션 A
BEGIN;
SELECT count(*) FROM t;   -- 10
-- (세션 B가 INSERT + COMMIT)
SELECT count(*) FROM t;   -- 11  ← 달라짐
COMMIT;
```

UPDATE 시 다른 트랜잭션이 잠근 행을 만나면 대기 후 **최신 버전을 다시 읽어** 조건을
재평가한다(EvalPlanQual). 이 때문에 예상 밖의 결과가 나올 수 있다.

### REPEATABLE READ / SERIALIZABLE 동작
트랜잭션 시작 시점의 스냅샷을 끝까지 유지한다. 충돌 시 에러가 발생한다.

```sql
-- 40001: could not serialize access due to concurrent update
-- 40001: could not serialize access due to read/write dependencies among transactions
```

**규칙: RR/SERIALIZABLE을 쓰면 애플리케이션에 재시도 로직이 반드시 있어야 한다.**

```sql
-- 재시도 의사 코드
for attempt in 1..3:
    try:
        BEGIN ISOLATION LEVEL SERIALIZABLE
        ... 작업 ...
        COMMIT
        break
    except SQLSTATE '40001' or '40P01':
        ROLLBACK
        sleep(random backoff)
```

## 4. 락 (Lock)

### 테이블 레벨 락 모드
| 모드 | 획득하는 명령 | 충돌 대상 |
|---|---|---|
| `ACCESS SHARE` | SELECT | ACCESS EXCLUSIVE |
| `ROW SHARE` | SELECT FOR UPDATE/SHARE | EXCLUSIVE, ACCESS EXCLUSIVE |
| `ROW EXCLUSIVE` | INSERT/UPDATE/DELETE/MERGE | SHARE 이상 |
| `SHARE UPDATE EXCLUSIVE` | VACUUM, ANALYZE, CREATE INDEX CONCURRENTLY | 자기 자신 이상 |
| `SHARE` | CREATE INDEX (비concurrent) | 쓰기 |
| `SHARE ROW EXCLUSIVE` | CREATE TRIGGER | 대부분 |
| `EXCLUSIVE` | REFRESH MAT VIEW CONCURRENTLY | SELECT 외 전부 |
| `ACCESS EXCLUSIVE` | ALTER/DROP TABLE, TRUNCATE, VACUUM FULL, REINDEX | **모든 것** |

```sql
LOCK TABLE t IN ACCESS EXCLUSIVE MODE;
LOCK TABLE t IN SHARE MODE NOWAIT;
```

**운영 핵심:** `ACCESS EXCLUSIVE` 락은 SELECT까지 막는다. 대기 큐 때문에 짧은 DDL도
서비스를 멈출 수 있으므로 반드시 `lock_timeout`을 건다.

```sql
SET lock_timeout = '3s';
ALTER TABLE big ADD COLUMN c text;
```

### 행 레벨 락
```sql
SELECT ... FOR UPDATE;          -- 배타. 다른 UPDATE/DELETE/FOR UPDATE 차단
SELECT ... FOR NO KEY UPDATE;   -- 약한 배타. FK 참조 삽입 허용
SELECT ... FOR SHARE;           -- 공유. 읽기끼리는 공존
SELECT ... FOR KEY SHARE;       -- 가장 약함. FK 검사용

SELECT ... FOR UPDATE NOWAIT;        -- 대기 없이 에러(55P03)
SELECT ... FOR UPDATE SKIP LOCKED;   -- 잠긴 행 건너뜀
```

**PostgreSQL에서 읽기는 쓰기를 막지 않고, 쓰기는 읽기를 막지 않는다** (MVCC).
`SELECT`는 `FOR UPDATE` 없이는 행 락을 잡지 않는다.

### 데드락
```sql
-- 40P01: deadlock detected
SHOW deadlock_timeout;   -- 기본 1s, 이후 자동 감지하고 한쪽을 abort
```
**예방: 모든 트랜잭션이 같은 순서로 자원을 잠그도록 한다.** (예: 항상 id 오름차순)

```sql
-- 정렬해서 잠그기
SELECT * FROM accounts WHERE id = ANY($1) ORDER BY id FOR UPDATE;
```

## 5. MVCC와 VACUUM

PostgreSQL은 UPDATE 시 기존 행을 지우지 않고 **새 버전을 추가**한다.
구 버전(dead tuple)은 VACUUM이 정리한다.

```sql
VACUUM t;                   -- dead tuple 회수 (공간은 OS로 반환 안 됨)
VACUUM (ANALYZE, VERBOSE) t;
VACUUM FULL t;              -- 테이블 재작성, 공간 반환. ACCESS EXCLUSIVE 락 — 운영 중 금지
ANALYZE t;                  -- 통계만 갱신

-- 팽창(bloat) 확인
SELECT relname, n_live_tup, n_dead_tup,
       round(n_dead_tup*100.0/NULLIF(n_live_tup+n_dead_tup,0), 1) AS dead_pct,
       last_autovacuum
FROM pg_stat_user_tables ORDER BY n_dead_tup DESC LIMIT 10;

-- 테이블별 autovacuum 튜닝
ALTER TABLE t SET (autovacuum_vacuum_scale_factor = 0.05);
```

**긴 트랜잭션은 VACUUM을 막는다.** 오래 열린 트랜잭션이 있으면 그 스냅샷 이후의
dead tuple을 회수할 수 없어 테이블이 팽창한다.

```sql
-- 오래된 트랜잭션 찾기
SELECT pid, now()-xact_start AS age, state, left(query,60)
FROM pg_stat_activity
WHERE xact_start IS NOT NULL ORDER BY xact_start LIMIT 10;

-- 방치 방지 설정
SET idle_in_transaction_session_timeout = '5min';
SET statement_timeout = '30s';
```

## 6. 타임아웃 설정

```sql
SET statement_timeout = '30s';                       -- 쿼리 최대 실행 시간
SET lock_timeout = '3s';                             -- 락 대기 최대 시간
SET idle_in_transaction_session_timeout = '5min';    -- 유휴 트랜잭션 종료
SET transaction_timeout = '1min';                    -- PG17+

-- 사용자/DB 단위 영구 설정
ALTER ROLE app_user SET statement_timeout = '30s';
ALTER DATABASE mydb SET lock_timeout = '3s';
```

## 7. DDL 트랜잭션

**PostgreSQL은 DDL도 트랜잭션 안에서 롤백된다** (Oracle/MySQL과 큰 차이).

```sql
BEGIN;
CREATE TABLE t (id int);
ALTER TABLE t ADD COLUMN c text;
ROLLBACK;   -- 테이블 생성 자체가 취소됨
```

**예외 (트랜잭션 블록 안에서 실행 불가):**
- `CREATE INDEX CONCURRENTLY` / `DROP INDEX CONCURRENTLY`
- `CREATE DATABASE` / `DROP DATABASE`
- `VACUUM` / `ANALYZE` (단독은 가능하나 VACUUM은 불가)
- `CREATE TABLESPACE`
- `ALTER TYPE ... ADD VALUE` (PG12+ 부터는 일부 허용)
- `REINDEX CONCURRENTLY`

마이그레이션 스크립트를 트랜잭션으로 감쌀 수 있다는 것은 큰 장점이다.

## 8. 2단계 커밋 (분산 트랜잭션)

```sql
BEGIN;
    ...
PREPARE TRANSACTION 'txn_id_1';

COMMIT PREPARED 'txn_id_1';
ROLLBACK PREPARED 'txn_id_1';

SELECT * FROM pg_prepared_xacts;
SHOW max_prepared_transactions;   -- 0이면 비활성 (기본값)
```
**방치된 prepared transaction은 VACUUM을 영구히 막는다.** 반드시 모니터링할 것.

## 9. 동시성 패턴

### 낙관적 잠금 (버전 컬럼)
```sql
UPDATE t SET a = 1, version = version + 1
WHERE id = 10 AND version = 3;
-- 영향 행 0 이면 다른 세션이 먼저 수정한 것 → 재시도
```

### 비관적 잠금
```sql
BEGIN;
SELECT * FROM t WHERE id = 10 FOR UPDATE;
UPDATE t SET a = 1 WHERE id = 10;
COMMIT;
```

### 원자적 증감 (락 불필요)
```sql
UPDATE counter SET cnt = cnt + 1 WHERE id = 1 RETURNING cnt;
```

### 작업 큐 (SKIP LOCKED)
```sql
BEGIN;
UPDATE jobs SET status='running', started_at=now()
WHERE id = (
    SELECT id FROM jobs WHERE status='pending'
    ORDER BY priority DESC, created_at
    LIMIT 1 FOR UPDATE SKIP LOCKED
)
RETURNING *;
COMMIT;
```

### 배치 중복 방지 (Advisory Lock)
```sql
SELECT pg_try_advisory_xact_lock(hashtext('nightly_batch'));
-- false면 이미 실행 중 → 종료
```

## 관련 문서

- [[PostgreSQL/03-DML|03. DML — SELECT / INSERT / UPDATE / DELETE / MERGE]] — FOR UPDATE 등 DML 수준 잠금
- [[PostgreSQL/14-TUNING|14. DB 튜닝 방법론]] — VACUUM·동시성 튜닝
- [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]] — 자동 커밋·DDL 트랜잭션 동작 차이
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
