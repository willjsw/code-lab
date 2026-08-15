---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - dml
  - select
  - insert
  - update
  - delete
  - merge
  - status/verified
aliases:
  - UPSERT
  - RETURNING
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]]  ·  **다음** [[PostgreSQL/04-JOIN-SUBQUERY|04. JOIN, 서브쿼리, CTE]]

# 03. DML — SELECT / INSERT / UPDATE / DELETE / MERGE

## 1. SELECT 전체 구조와 실행 순서

```sql
SELECT DISTINCT [ON (expr)] select_list
FROM        table_ref
WHERE       condition
GROUP BY    expr
HAVING      condition
WINDOW      w AS (...)
ORDER BY    expr
LIMIT n OFFSET m
FOR UPDATE
```

**논리적 실행 순서** (작성 순서와 다름):
```
FROM/JOIN → WHERE → GROUP BY → HAVING → WINDOW → SELECT → DISTINCT → ORDER BY → LIMIT
```

이 순서 때문에 생기는 규칙:
```sql
-- WHERE 에서는 SELECT 별칭 사용 불가 (아직 계산 전)
SELECT price * 2 AS dbl FROM t WHERE dbl > 100;   -- 에러

-- ORDER BY / GROUP BY 에서는 별칭 사용 가능 (PostgreSQL 확장)
SELECT price * 2 AS dbl FROM t ORDER BY dbl;      -- OK
SELECT price * 2 AS dbl FROM t GROUP BY dbl;      -- OK (PostgreSQL만)

-- WHERE에서 쓰려면 서브쿼리나 LATERAL로
SELECT * FROM (SELECT price*2 AS dbl FROM t) s WHERE dbl > 100;
```

## 2. DISTINCT / DISTINCT ON

```sql
SELECT DISTINCT dept FROM emp;

-- DISTINCT ON: PostgreSQL 고유. 그룹별 첫 행 (window 함수보다 간결/빠름)
-- ORDER BY 의 선두가 DISTINCT ON 표현식과 일치해야 함
SELECT DISTINCT ON (user_id)
       user_id, order_no, created_at
FROM orders
ORDER BY user_id, created_at DESC;   -- 사용자별 최신 주문 1건
```

## 3. LIMIT / OFFSET / FETCH

```sql
SELECT * FROM t ORDER BY id LIMIT 10 OFFSET 20;            -- PostgreSQL 문법
SELECT * FROM t ORDER BY id OFFSET 20 ROWS FETCH FIRST 10 ROWS ONLY;  -- 표준
SELECT * FROM t ORDER BY score DESC FETCH FIRST 10 ROWS WITH TIES;    -- 동점 포함
```

**대용량 페이지네이션은 OFFSET 대신 keyset 페이징** (OFFSET은 앞 행을 전부 읽음):
```sql
-- 나쁨: OFFSET 100000 은 10만 행 스캔
SELECT * FROM t ORDER BY id LIMIT 20 OFFSET 100000;

-- 좋음: keyset (seek method)
SELECT * FROM t WHERE id > :last_id ORDER BY id LIMIT 20;
-- 복합 키
SELECT * FROM t WHERE (created_at, id) < (:last_ts, :last_id)
ORDER BY created_at DESC, id DESC LIMIT 20;
```

## 4. GROUP BY 확장

```sql
SELECT dept, job, sum(sal) FROM emp GROUP BY dept, job;

-- ROLLUP: 계층적 소계 + 총계
SELECT dept, job, sum(sal) FROM emp GROUP BY ROLLUP (dept, job);
-- (dept,job), (dept), () 조합

-- CUBE: 모든 조합
SELECT dept, job, sum(sal) FROM emp GROUP BY CUBE (dept, job);
-- (dept,job), (dept), (job), ()

-- GROUPING SETS: 원하는 조합만
SELECT dept, job, sum(sal) FROM emp
GROUP BY GROUPING SETS ((dept), (job), ());

-- GROUPING() 으로 소계 행 구분 (1이면 집계된 열)
SELECT CASE WHEN GROUPING(dept)=1 THEN '전체' ELSE dept END AS dept,
       sum(sal)
FROM emp GROUP BY ROLLUP (dept);
```

### HAVING vs WHERE
```sql
-- WHERE: 그룹핑 전 행 필터 (빠름)
-- HAVING: 그룹핑 후 집계 결과 필터
SELECT dept, count(*) FROM emp
WHERE hire_date > '2020-01-01'    -- 먼저 걸러냄
GROUP BY dept
HAVING count(*) > 5;              -- 집계 후 걸러냄
```

## 5. 집합 연산

```sql
SELECT a FROM t1 UNION     SELECT a FROM t2;   -- 중복 제거 (정렬 비용 발생)
SELECT a FROM t1 UNION ALL SELECT a FROM t2;   -- 중복 유지 (빠름, 기본 선택)
SELECT a FROM t1 INTERSECT SELECT a FROM t2;   -- 교집합
SELECT a FROM t1 EXCEPT    SELECT a FROM t2;   -- 차집합 (Oracle의 MINUS)

-- 규칙: 컬럼 개수/타입 호환 필요. 컬럼명은 첫 SELECT 기준
-- ORDER BY / LIMIT 은 전체에 한 번만 (마지막에)
(SELECT a FROM t1 ORDER BY a LIMIT 5)
UNION ALL
(SELECT a FROM t2 ORDER BY a LIMIT 5)
ORDER BY a;
```

## 6. INSERT

```sql
INSERT INTO t (a, b) VALUES (1, 'x');
INSERT INTO t (a, b) VALUES (1,'x'), (2,'y'), (3,'z');   -- 다중 행
INSERT INTO t (a, b) SELECT a, b FROM src WHERE ...;      -- SELECT 삽입
INSERT INTO t DEFAULT VALUES;

-- RETURNING: 삽입 결과 즉시 반환 (PostgreSQL 고유, 매우 유용)
INSERT INTO t (a) VALUES (1) RETURNING id, created_at;
INSERT INTO t (a) SELECT ... RETURNING *;
```

### UPSERT — ON CONFLICT
```sql
-- 충돌 시 무시
INSERT INTO users (email, name) VALUES ('a@b.c', 'kim')
ON CONFLICT (email) DO NOTHING;

-- 충돌 시 갱신 (EXCLUDED = 삽입하려던 행)
INSERT INTO users (email, name, updated_at)
VALUES ('a@b.c', 'kim', now())
ON CONFLICT (email) DO UPDATE
   SET name = EXCLUDED.name,
       updated_at = EXCLUDED.updated_at
 WHERE users.name IS DISTINCT FROM EXCLUDED.name   -- 변경 있을 때만
RETURNING *;

-- 제약조건 이름으로 지정
ON CONFLICT ON CONSTRAINT users_email_uk DO UPDATE SET ...

-- 부분 인덱스 대상
ON CONFLICT (email) WHERE status = 'active' DO NOTHING;
```

**ON CONFLICT 주의점**
- 충돌 대상은 **UNIQUE 제약 또는 UNIQUE 인덱스**여야 함 (일반 인덱스 불가)
- `DO NOTHING`은 `RETURNING`이 아무것도 반환하지 않음 → 삽입 여부 판단 시 주의
- 같은 `INSERT` 문 안에서 **같은 키가 두 번** 나오면 에러 (`ON CONFLICT DO UPDATE cannot affect row a second time`) → 미리 dedup 필요

```sql
-- dedup 후 upsert
INSERT INTO users (email, name)
SELECT DISTINCT ON (email) email, name FROM staging ORDER BY email, updated_at DESC
ON CONFLICT (email) DO UPDATE SET name = EXCLUDED.name;
```

## 7. UPDATE

```sql
UPDATE t SET a = 1, b = 'x' WHERE id = 10;

-- 다중 컬럼 동시 할당
UPDATE t SET (a, b) = (1, 'x') WHERE id = 10;
UPDATE t SET (a, b) = (SELECT x, y FROM s WHERE s.id = t.id);

-- FROM 절 조인 UPDATE (PostgreSQL 고유 — Oracle과 문법 다름)
UPDATE orders o
   SET user_name = u.name
  FROM users u
 WHERE o.user_id = u.id
   AND o.user_name IS NULL;

-- VALUES 리스트로 대량 개별 갱신
UPDATE t SET val = v.val
FROM (VALUES (1,'a'), (2,'b')) AS v(id, val)
WHERE t.id = v.id;

UPDATE t SET a = a + 1 WHERE id = 1 RETURNING a;
```

**주의:** `UPDATE ... FROM`에서 조인 결과가 여러 행이면 **어느 행이 적용될지 비결정적**.
중복 없는 조인인지 확인할 것.

## 8. DELETE

```sql
DELETE FROM t WHERE id = 1;
DELETE FROM t;                    -- 전체 (TRUNCATE가 훨씬 빠름)

-- USING 조인 DELETE
DELETE FROM orders o USING users u
 WHERE o.user_id = u.id AND u.status = 'deleted';

DELETE FROM t WHERE id IN (SELECT id FROM other);
DELETE FROM t WHERE id = 1 RETURNING *;

-- 대량 삭제는 배치로 (락 시간/WAL 폭증 방지)
DELETE FROM t WHERE ctid IN (
    SELECT ctid FROM t WHERE created_at < '2020-01-01' LIMIT 10000
);
```

## 9. MERGE (PG15+)

```sql
MERGE INTO target t
USING source s ON t.id = s.id
WHEN MATCHED AND s.deleted THEN
    DELETE
WHEN MATCHED THEN
    UPDATE SET val = s.val, updated_at = now()
WHEN NOT MATCHED THEN
    INSERT (id, val) VALUES (s.id, s.val)
WHEN NOT MATCHED BY SOURCE THEN      -- PG17+
    UPDATE SET status = 'orphan';
```

**MERGE vs ON CONFLICT**
| | MERGE | ON CONFLICT |
|---|---|---|
| 버전 | PG15+ | PG9.5+ |
| 대상 지정 | 임의 조인 조건 | UNIQUE 제약 필요 |
| 동시성 | 원자적 아님 (직렬화 에러 가능) | 원자적 upsert |
| DELETE | 가능 | 불가 |
| RETURNING | PG17+ | 지원 |

단순 upsert면 `ON CONFLICT`를 쓰는 것이 동시성 면에서 안전하다.

## 10. RETURNING 활용

```sql
-- CTE와 조합: 삭제한 행을 아카이브로 이동
WITH deleted AS (
    DELETE FROM orders WHERE created_at < '2020-01-01' RETURNING *
)
INSERT INTO orders_archive SELECT * FROM deleted;

-- 삽입 후 자식 테이블에 연쇄 삽입
WITH new_order AS (
    INSERT INTO orders (user_id) VALUES (1) RETURNING id
)
INSERT INTO order_items (order_id, sku)
SELECT id, 'ABC' FROM new_order;

-- PG18+: OLD/NEW 참조
UPDATE t SET v = v + 1 RETURNING OLD.v AS before, NEW.v AS after;
```

**CTE 내 DML 주의:** 같은 문장 안의 DML CTE들은 **동일 스냅샷**을 본다. 앞 CTE의 변경이
뒤 CTE에 보이지 않는다.

## 11. VALUES 절 (독립 테이블처럼 사용)

```sql
SELECT * FROM (VALUES (1,'a'), (2,'b')) AS t(id, name);

-- 조인 대상으로
SELECT * FROM users u
JOIN (VALUES ('active','활성'), ('inactive','비활성')) AS m(code, label)
  ON u.status = m.code;
```

## 12. FOR UPDATE — 행 잠금

```sql
SELECT * FROM t WHERE id = 1 FOR UPDATE;             -- 배타 잠금, 대기
SELECT * FROM t WHERE id = 1 FOR UPDATE NOWAIT;      -- 잠겨 있으면 즉시 에러
SELECT * FROM t WHERE id = 1 FOR UPDATE SKIP LOCKED; -- 잠긴 행 건너뜀 (큐 구현)
SELECT * FROM t FOR NO KEY UPDATE;                   -- 약한 잠금 (FK 삽입 허용)
SELECT * FROM t FOR SHARE;                           -- 공유 잠금
SELECT * FROM a JOIN b ON ... FOR UPDATE OF a;       -- 특정 테이블만

-- 작업 큐 패턴
UPDATE jobs SET status='running'
WHERE id IN (
    SELECT id FROM jobs WHERE status='pending'
    ORDER BY created_at LIMIT 10 FOR UPDATE SKIP LOCKED
)
RETURNING *;
```

## 관련 문서

- [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]] — 대상 테이블·제약조건 정의
- [[PostgreSQL/04-JOIN-SUBQUERY|04. JOIN, 서브쿼리, CTE]] — SELECT의 JOIN·서브쿼리 상세
- [[PostgreSQL/06-WINDOW|06. 윈도우 함수]] — GROUP BY로 안 되는 행별 집계
- [[PostgreSQL/10-TRANSACTION|10. 트랜잭션, 격리 수준, 락]] — FOR UPDATE 행 잠금과 격리 수준
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
