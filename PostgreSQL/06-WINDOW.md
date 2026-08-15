---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - window-function
  - over
  - frame
  - ranking
  - status/verified
aliases:
  - 윈도우 함수
  - OVER 절
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]]  ·  **다음** [[PostgreSQL/07-PLPGSQL|07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)]]

# 06. 윈도우 함수

## 1. 기본 구조

```sql
함수명(인자) OVER (
    PARTITION BY 그룹핑_컬럼
    ORDER BY 정렬_컬럼
    프레임_절
)
```

GROUP BY와 달리 **행을 줄이지 않고** 집계 결과를 각 행에 붙인다.

```sql
SELECT
    dept, name, salary,
    sum(salary)  OVER (PARTITION BY dept)                AS dept_total,
    avg(salary)  OVER (PARTITION BY dept)                AS dept_avg,
    salary - avg(salary) OVER (PARTITION BY dept)        AS diff,
    rank()       OVER (PARTITION BY dept ORDER BY salary DESC) AS rnk
FROM emp;
```

## 2. 순위 함수

```sql
row_number() OVER (ORDER BY x)     -- 1,2,3,4 (항상 고유)
rank()       OVER (ORDER BY x)     -- 1,2,2,4 (동점 후 건너뜀)
dense_rank() OVER (ORDER BY x)     -- 1,2,2,3 (동점 후 연속)
percent_rank() OVER (ORDER BY x)   -- (rank-1)/(전체-1), 0~1
cume_dist()  OVER (ORDER BY x)     -- 누적 분포, 0 초과 1 이하
ntile(4)     OVER (ORDER BY x)     -- 4분위 그룹 번호
```

### 그룹별 상위 N건
```sql
-- 방법 1: row_number (동점 처리 없음)
SELECT * FROM (
    SELECT *, row_number() OVER (PARTITION BY dept ORDER BY salary DESC) rn
    FROM emp
) t WHERE rn <= 3;

-- 방법 2: rank (동점 모두 포함)
... WHERE rnk <= 3;

-- 방법 3: DISTINCT ON (1건만 필요할 때 가장 빠름)
SELECT DISTINCT ON (dept) * FROM emp ORDER BY dept, salary DESC;

-- 방법 4: LATERAL (dept 목록이 작고 emp에 인덱스 있을 때 가장 빠름)
SELECT e.* FROM depts d
CROSS JOIN LATERAL (
    SELECT * FROM emp WHERE dept = d.name ORDER BY salary DESC LIMIT 3
) e;
```

## 3. 값 접근 함수

```sql
lag(x)              OVER (ORDER BY t)   -- 이전 행 값
lag(x, 2, 0)        OVER (ORDER BY t)   -- 2행 전, 없으면 0
lead(x)             OVER (ORDER BY t)   -- 다음 행 값
first_value(x)      OVER (...)          -- 프레임 첫 값
last_value(x)       OVER (...)          -- 프레임 마지막 값
nth_value(x, 2)     OVER (...)          -- 프레임 n번째 값

-- 전일 대비 증감
SELECT dt, amt,
       lag(amt) OVER (ORDER BY dt) AS prev,
       amt - lag(amt) OVER (ORDER BY dt) AS diff,
       round((amt - lag(amt) OVER (ORDER BY dt)) * 100.0
             / NULLIF(lag(amt) OVER (ORDER BY dt), 0), 2) AS pct
FROM daily;
```

**`last_value` 함정:** 기본 프레임이 `RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW`
이므로 `last_value`가 현재 행을 반환한다. 프레임을 명시해야 한다.
```sql
last_value(x) OVER (ORDER BY t ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING)
-- 또는 역순 first_value 사용
first_value(x) OVER (ORDER BY t DESC)
```

## 4. 프레임 절

```sql
{ROWS | RANGE | GROUPS} BETWEEN 시작 AND 끝

-- 경계 지정자
UNBOUNDED PRECEDING     -- 파티션 시작
n PRECEDING             -- n행/n값 앞
CURRENT ROW
n FOLLOWING
UNBOUNDED FOLLOWING     -- 파티션 끝
```

| 모드 | 기준 |
|---|---|
| `ROWS` | 물리적 행 개수 |
| `RANGE` | ORDER BY 값의 범위 (동점은 한 덩어리) |
| `GROUPS` | 동점 그룹 개수 (PG11+) |

**기본 프레임:**
- `ORDER BY` 있음 → `RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW`
- `ORDER BY` 없음 → `RANGE BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING` (파티션 전체)

```sql
-- 누적 합계
sum(amt) OVER (ORDER BY dt ROWS UNBOUNDED PRECEDING)

-- 이동 평균 (직전 2행 + 현재 = 3일)
avg(amt) OVER (ORDER BY dt ROWS BETWEEN 2 PRECEDING AND CURRENT ROW)

-- 앞뒤 1행 포함
sum(amt) OVER (ORDER BY dt ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING)

-- 날짜 기준 7일 이동 (RANGE + interval, PG11+)
sum(amt) OVER (ORDER BY dt RANGE BETWEEN interval '6 days' PRECEDING AND CURRENT ROW)

-- EXCLUDE 옵션 (PG11+)
sum(x) OVER (... EXCLUDE CURRENT ROW)
sum(x) OVER (... EXCLUDE TIES)
sum(x) OVER (... EXCLUDE GROUP)
```

**ROWS vs RANGE 차이 (동점이 있을 때):**
```sql
-- dt 가 중복되는 데이터
sum(x) OVER (ORDER BY dt)                          -- RANGE: 같은 dt 전부 포함
sum(x) OVER (ORDER BY dt ROWS UNBOUNDED PRECEDING) -- ROWS: 현재 행까지만
```

## 5. WINDOW 절 — 재사용

```sql
SELECT name, salary,
       rank()       OVER w,
       sum(salary)  OVER w,
       avg(salary)  OVER w
FROM emp
WINDOW w AS (PARTITION BY dept ORDER BY salary DESC);

-- 기존 윈도우 확장
WINDOW w  AS (PARTITION BY dept),
       w2 AS (w ORDER BY salary DESC);
```

## 6. 사용 제약

```sql
-- 윈도우 함수는 SELECT 와 ORDER BY 에서만 사용 가능
-- WHERE / GROUP BY / HAVING 에서는 불가 (계산 시점이 그 이후)
SELECT * FROM emp WHERE row_number() OVER (...) <= 3;   -- 에러

-- 서브쿼리나 CTE로 감싸야 함
SELECT * FROM (SELECT *, row_number() OVER (...) rn FROM emp) t WHERE rn <= 3;
-- 또는 QUALIFY 대용 (PostgreSQL에는 QUALIFY 없음)
```

## 7. 집계 + 윈도우 조합

집계 후에 윈도우 함수를 적용할 수 있다 (윈도우는 GROUP BY 이후 실행).
```sql
SELECT dept,
       sum(salary) AS dept_sum,
       sum(sum(salary)) OVER () AS total_sum,               -- 전체 합
       round(sum(salary) * 100.0 / sum(sum(salary)) OVER (), 2) AS pct
FROM emp
GROUP BY dept;
```

## 8. 실전 패턴

```sql
-- 연속 구간(gaps and islands) 찾기
SELECT min(dt), max(dt), count(*)
FROM (
    SELECT dt, dt - (row_number() OVER (ORDER BY dt))::int * interval '1 day' AS grp
    FROM attendance
) t GROUP BY grp;

-- 그룹 내 첫 행 여부
SELECT *, row_number() OVER (PARTITION BY user_id ORDER BY dt) = 1 AS is_first
FROM events;

-- 중복 제거 (최신 1건만 남기고 삭제)
DELETE FROM t WHERE ctid IN (
    SELECT ctid FROM (
        SELECT ctid, row_number() OVER (PARTITION BY email ORDER BY id DESC) rn
        FROM t
    ) s WHERE rn > 1
);

-- 값이 바뀐 시점 표시
SELECT *, status IS DISTINCT FROM lag(status) OVER (PARTITION BY id ORDER BY dt) AS changed
FROM history;
```

## 관련 문서

- [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]] — 집계 함수 기본형
- [[PostgreSQL/03-DML|03. DML — SELECT / INSERT / UPDATE / DELETE / MERGE]] — GROUP BY 확장과의 구분
- [[PostgreSQL/04-JOIN-SUBQUERY|04. JOIN, 서브쿼리, CTE]] — CTE로 윈도우 결과 재필터링
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
