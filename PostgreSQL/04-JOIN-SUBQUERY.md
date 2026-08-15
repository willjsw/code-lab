---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - join
  - subquery
  - cte
  - lateral
  - status/verified
aliases:
  - LATERAL JOIN
  - CTE
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/03-DML|03. DML — SELECT / INSERT / UPDATE / DELETE / MERGE]]  ·  **다음** [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]]

# 04. JOIN, 서브쿼리, CTE

## 1. JOIN 종류

```sql
-- INNER JOIN: 양쪽 모두 매칭되는 행
SELECT * FROM a INNER JOIN b ON a.id = b.a_id;
SELECT * FROM a JOIN b ON a.id = b.a_id;         -- INNER 생략 가능

-- LEFT OUTER JOIN: 왼쪽 전부 + 매칭되는 오른쪽 (없으면 NULL)
SELECT * FROM a LEFT JOIN b ON a.id = b.a_id;

-- RIGHT / FULL OUTER
SELECT * FROM a RIGHT JOIN b ON a.id = b.a_id;
SELECT * FROM a FULL  JOIN b ON a.id = b.a_id;

-- CROSS JOIN: 카테시안 곱
SELECT * FROM a CROSS JOIN b;
SELECT * FROM a, b;   -- 동일 (구식 표기)

-- USING: 컬럼명이 같을 때. 조인 컬럼이 한 번만 출력됨
SELECT * FROM a JOIN b USING (id);

-- NATURAL JOIN: 같은 이름 컬럼 전부 자동 조인. 절대 쓰지 말 것
-- (컬럼 추가되면 조인 조건이 조용히 바뀜)
```

### LEFT JOIN 필터링 함정
```sql
-- 잘못: WHERE 절에 오른쪽 조건 → INNER JOIN이 되어버림
SELECT * FROM a LEFT JOIN b ON a.id = b.a_id WHERE b.status = 'x';

-- 올바름: ON 절에 넣거나
SELECT * FROM a LEFT JOIN b ON a.id = b.a_id AND b.status = 'x';
-- NULL 허용하거나
SELECT * FROM a LEFT JOIN b ON a.id = b.a_id
WHERE b.status = 'x' OR b.id IS NULL;

-- ANTI JOIN (b에 없는 a만)
SELECT * FROM a LEFT JOIN b ON a.id = b.a_id WHERE b.a_id IS NULL;
```

## 2. LATERAL JOIN — PostgreSQL 고유

오른쪽 서브쿼리가 **왼쪽 테이블의 컬럼을 참조**할 수 있다. 그룹별 상위 N건에 필수.

```sql
-- 사용자별 최신 주문 3건
SELECT u.id, u.name, o.order_no, o.created_at
FROM users u
CROSS JOIN LATERAL (
    SELECT order_no, created_at
    FROM orders
    WHERE user_id = u.id          -- 바깥 u 참조 (LATERAL 없으면 불가)
    ORDER BY created_at DESC
    LIMIT 3
) o;

-- LEFT JOIN LATERAL: 주문 없는 사용자도 포함 (ON true 필수)
SELECT u.id, o.order_no
FROM users u
LEFT JOIN LATERAL (
    SELECT order_no FROM orders WHERE user_id = u.id ORDER BY created_at DESC LIMIT 1
) o ON true;

-- 함수 호출에서는 LATERAL 키워드 생략 가능
SELECT u.id, t.*
FROM users u, unnest(u.tags) AS t(tag);
```

## 3. 서브쿼리

### 스칼라 서브쿼리 (1행 1열)
```sql
SELECT id, (SELECT count(*) FROM orders o WHERE o.user_id = u.id) AS cnt
FROM users u;
-- 행마다 실행 → 대량이면 LEFT JOIN + GROUP BY 로 대체
```

### IN / NOT IN / EXISTS
```sql
SELECT * FROM users WHERE id IN (SELECT user_id FROM orders);
SELECT * FROM users u WHERE EXISTS (SELECT 1 FROM orders o WHERE o.user_id = u.id);
```

**`NOT IN`은 NULL 함정이 있다.** 서브쿼리 결과에 NULL이 하나라도 있으면 **전체가 빈 결과**.
```sql
SELECT * FROM users WHERE id NOT IN (SELECT user_id FROM orders);
-- orders.user_id 에 NULL 있으면 결과 0건

-- 안전한 대안
SELECT * FROM users u WHERE NOT EXISTS (SELECT 1 FROM orders o WHERE o.user_id = u.id);
-- 또는
... NOT IN (SELECT user_id FROM orders WHERE user_id IS NOT NULL);
```

### ANY / ALL / SOME
```sql
SELECT * FROM t WHERE x > ALL (SELECT y FROM s);   -- 모든 y보다 큼
SELECT * FROM t WHERE x = ANY (SELECT y FROM s);   -- IN 과 동일
SELECT * FROM t WHERE x = ANY ('{1,2,3}'::int[]);  -- 배열에도 사용 (PG 고유)
```

### 행 생성자 (Row Constructor)
```sql
SELECT * FROM t WHERE (a, b) = (1, 'x');
SELECT * FROM t WHERE (a, b) IN ((1,'x'), (2,'y'));
SELECT * FROM t WHERE (created_at, id) > ('2026-01-01', 100);  -- keyset 페이징
```

## 4. CTE (WITH 절)

```sql
WITH active AS (
    SELECT * FROM users WHERE status = 'active'
),
stats AS (
    SELECT user_id, count(*) cnt FROM orders GROUP BY user_id
)
SELECT a.*, s.cnt
FROM active a LEFT JOIN stats s ON a.id = s.user_id;
```

### 최적화 경계 — 버전별 동작 차이 (중요)
- **PG11 이하**: CTE는 항상 **최적화 방벽(materialize)**. 별도 실행 후 결과 저장
- **PG12+**: 부작용 없고 1회만 참조되면 **자동 인라인**. 나머지는 materialize

```sql
WITH x AS MATERIALIZED     (SELECT ...)  -- 강제 구체화 (PG12+)
WITH x AS NOT MATERIALIZED (SELECT ...)  -- 강제 인라인 (PG12+)
```

성능 문제 시 이 힌트로 조정한다. PG12 미만 코드를 이관할 때 CTE가 인라인되면서
플랜이 바뀌는 사례가 흔하다.

### 재귀 CTE
```sql
-- 조직도 계층 탐색
WITH RECURSIVE tree AS (
    -- 앵커: 시작점
    SELECT id, parent_id, name, 1 AS depth, ARRAY[id] AS path
    FROM dept WHERE parent_id IS NULL

    UNION ALL   -- UNION ALL 권장 (UNION은 매 회 중복 제거로 느림)

    -- 재귀: tree 자신을 참조
    SELECT d.id, d.parent_id, d.name, t.depth + 1, t.path || d.id
    FROM dept d
    JOIN tree t ON d.parent_id = t.id
    WHERE NOT d.id = ANY(t.path)   -- 순환 방지
      AND t.depth < 100            -- 안전장치
)
SELECT * FROM tree ORDER BY path;
```

```sql
-- CYCLE 절 (PG14+) — 순환 자동 감지
WITH RECURSIVE tree AS (
    SELECT id, parent_id FROM dept WHERE id = 1
    UNION ALL
    SELECT d.id, d.parent_id FROM dept d JOIN tree t ON d.parent_id = t.id
) CYCLE id SET is_cycle USING path
SELECT * FROM tree WHERE NOT is_cycle;

-- SEARCH 절 (PG14+) — 탐색 순서 지정
) SEARCH DEPTH FIRST BY id SET ord
) SEARCH BREADTH FIRST BY id SET ord
```

```sql
-- 연속 숫자/날짜 생성 (재귀 대신 generate_series 권장)
SELECT generate_series(1, 10);
SELECT generate_series('2026-01-01'::date, '2026-12-31', '1 month');
```

### DML CTE
```sql
WITH moved AS (
    DELETE FROM staging WHERE processed RETURNING *
)
INSERT INTO main SELECT * FROM moved;
```
주의: DML CTE는 동일 스냅샷을 보며, 메인 쿼리에서 참조하지 않아도 **항상 실행된다**.

## 5. 조인 알고리즘 (플래너가 선택)

| 방식 | 적합한 상황 |
|---|---|
| Nested Loop | 한쪽이 매우 작고, 다른 쪽에 인덱스 존재 |
| Hash Join | 등호 조인, 중간~대량, 해시 테이블 메모리 수용 가능 |
| Merge Join | 양쪽이 조인 키로 이미 정렬됨 (인덱스 스캔) |

```sql
-- 특정 방식 강제 (진단용. 운영 코드에 남기지 말 것)
SET enable_nestloop = off;
SET enable_hashjoin = off;
SET enable_mergejoin = off;
RESET ALL;
```
PostgreSQL에는 **쿼리 힌트가 없다**(`pg_hint_plan` 확장 필요). 통계와 인덱스로 유도하는 것이 정석.

## 6. 조인 순서 제어

```sql
-- 조인 테이블이 많으면 플래너가 탐색을 포기 (GEQO)
SHOW join_collapse_limit;  -- 기본 8
SHOW from_collapse_limit;  -- 기본 8
SHOW geqo_threshold;       -- 기본 12

-- 명시 순서를 강제하려면
SET join_collapse_limit = 1;   -- 작성한 JOIN 순서 그대로 사용
```

## 관련 문서

- [[PostgreSQL/03-DML|03. DML — SELECT / INSERT / UPDATE / DELETE / MERGE]] — SELECT 전체 구조와 실행 순서
- [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]] — 집합 반환 함수와 LATERAL 조합
- [[PostgreSQL/11-PERFORMANCE|11. 성능 — EXPLAIN, 인덱스 전략, 통계]] — 조인 알고리즘 선택과 플래너 판단
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
