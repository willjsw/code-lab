---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - convention
  - naming
  - anti-pattern
  - migration
  - code-review
  - status/verified
aliases:
  - 네이밍 컨벤션
  - SQL 안티패턴
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/11-PERFORMANCE|11. 성능 — EXPLAIN, 인덱스 전략, 통계]]  ·  **다음** [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]]

# 12. 코딩 컨벤션과 안티패턴

## 1. 네이밍

### 기본 원칙
- **전부 소문자 `snake_case`.** 큰따옴표 식별자 금지
- 63바이트 제한 (초과분은 경고 없이 잘림)
- 예약어 사용 금지 (`user`, `order`, `group`, `table`, `end`, `desc`, `type` 등)

```sql
-- user 는 예약어. users 또는 app_user 로
CREATE TABLE users (...);      -- OK
CREATE TABLE "user" (...);     -- 매번 따옴표 필요. 금지
```

### 객체별 규칙
| 객체 | 규칙 | 예시 |
|---|---|---|
| 테이블 | 복수형 명사 (팀 컨벤션 따를 것) | `users`, `order_items` |
| 컬럼 | 단수 명사 | `email`, `created_at` |
| PK | `id` 또는 `<table>_id` | `id` |
| FK | `<참조테이블단수>_id` | `user_id` |
| 불리언 | `is_`/`has_` 접두 | `is_active`, `has_paid` |
| 타임스탬프 | `_at` 접미 | `created_at`, `deleted_at` |
| 날짜 | `_date`/`_on` 접미 | `birth_date` |
| 인덱스 | `idx_<table>_<cols>` | `idx_users_email` |
| 유니크 | `uk_<table>_<cols>` | `uk_users_email` |
| PK 제약 | `pk_<table>` | `pk_users` |
| FK 제약 | `fk_<table>_<ref>` | `fk_orders_users` |
| CHECK | `ck_<table>_<rule>` | `ck_users_age` |
| 함수 | `fn_<동작>` | `fn_calc_fee` |
| 프로시저 | `sp_<동작>` | `sp_batch_process` |
| 트리거 | `trg_<table>_<이벤트>` | `trg_users_updated` |
| 뷰 | `v_<이름>` | `v_active_users` |
| 구체화뷰 | `mv_<이름>` | `mv_daily_stat` |
| 시퀀스 | `seq_<이름>` | `seq_order_no` |

### 변수 접두 (PL/pgSQL)
```sql
CREATE FUNCTION fn_x(p_user_id bigint)     -- p_ : 파라미터
RETURNS int LANGUAGE plpgsql AS $$
DECLARE
    v_count int;                            -- v_ : 지역 변수
    c_limit constant int := 100;            -- c_ : 상수
BEGIN
```
**접두사는 선택이 아니라 필수에 가깝다.** 변수명과 컬럼명이 같으면 PL/pgSQL이
변수를 우선하여 조용히 잘못된 결과가 나온다.

```sql
-- 위험한 예
DECLARE user_id bigint := 1;
BEGIN
    DELETE FROM orders WHERE user_id = user_id;   -- 항상 참! 전체 삭제됨
END;

-- 방어책 1: 접두사
DECLARE v_user_id bigint := 1;
    DELETE FROM orders WHERE user_id = v_user_id;

-- 방어책 2: 충돌 시 에러 발생시키기
#variable_conflict error       -- 함수 본문 최상단에 선언
-- 또는 use_column / use_variable
```

## 2. 포맷팅

```sql
-- 키워드 대문자, 식별자 소문자
SELECT u.id,
       u.email,
       count(o.id) AS order_cnt
  FROM users u
  LEFT JOIN orders o ON o.user_id = u.id
 WHERE u.status = 'active'
   AND u.created_at >= '2026-01-01'
 GROUP BY u.id, u.email
HAVING count(o.id) > 0
 ORDER BY order_cnt DESC
 LIMIT 10;

-- 컬럼이 많으면 한 줄에 하나
-- JOIN 조건은 ON 절에, 필터는 WHERE 절에
-- 테이블 별칭은 짧고 일관되게 (u, o, oi)
-- 서브쿼리보다 CTE로 이름을 붙여 가독성 확보
```

### INSERT는 컬럼명 명시
```sql
INSERT INTO users (email, name) VALUES ('a@b.c', 'kim');   -- OK
INSERT INTO users VALUES ('a@b.c', 'kim');                 -- 컬럼 순서 바뀌면 붕괴
```

### 명시적 캐스팅
```sql
WHERE id = $1::bigint
WHERE created_at >= $1::timestamptz
```
암묵 캐스팅은 인덱스 미사용의 흔한 원인이다.

## 3. 안티패턴

### `SELECT *` 남용
```sql
-- 컬럼 추가 시 애플리케이션이 깨지고, 불필요한 I/O가 발생하며,
-- Index Only Scan 기회를 잃는다
SELECT id, email FROM users;    -- 필요한 것만
```
단, `SELECT *`가 정당한 경우: `EXISTS(SELECT 1 ...)`, CTE 중간 단계, 임시 조회.

### `NOT IN` + NULL
```sql
-- 서브쿼리에 NULL 하나만 있어도 결과가 0건이 된다
SELECT * FROM a WHERE id NOT IN (SELECT b_id FROM b);        -- 위험
SELECT * FROM a WHERE NOT EXISTS (SELECT 1 FROM b WHERE b.b_id = a.id);  -- 안전
```

### `char(n)` 사용
공백 패딩 때문에 비교/길이 계산이 직관과 다르다. `text` 또는 `varchar(n)`을 쓴다.

### 금액에 `float`
```sql
amount float8       -- 절대 금지
amount numeric(15,2)
```

### `timestamp` (타임존 없음) 기본 사용
글로벌/서머타임 환경에서 반드시 문제가 된다. **`timestamptz`가 기본**이어야 한다.

### EAV (Entity-Attribute-Value)
```sql
-- 안티패턴
CREATE TABLE attrs (entity_id int, key text, value text);
-- PostgreSQL에서는 jsonb 컬럼이 훨씬 낫다
CREATE TABLE t (id int, attrs jsonb);
```

### 행 단위 루프
```sql
-- 나쁨
FOR r IN SELECT * FROM t LOOP
    UPDATE t2 SET x = r.x WHERE id = r.id;
END LOOP;

-- 좋음
UPDATE t2 SET x = t.x FROM t WHERE t2.id = t.id;
```

### 트리거 남용
비즈니스 로직을 트리거에 숨기면 디버깅이 어렵고, 연쇄 트리거로 성능이 무너진다.
`updated_at` 자동 갱신, 감사 로그 정도로 제한한다.

### 과도한 인덱스
쓰기 성능과 디스크를 갉아먹는다. `idx_scan = 0` 인덱스는 정기적으로 제거한다.

### 문자열 연결 동적 SQL
```sql
EXECUTE 'SELECT * FROM t WHERE n = ''' || p_n || '''';   -- 인젝션 취약
EXECUTE 'SELECT * FROM t WHERE n = $1' USING p_n;        -- 안전
EXECUTE format('SELECT * FROM %I WHERE n = $1', p_tbl) USING p_n;
```

### `SECURITY DEFINER` + search_path 미지정
권한 상승 공격 경로가 된다. 반드시 `SET search_path`를 붙인다.

### 긴 트랜잭션
VACUUM을 막아 테이블 팽창을 유발한다. 사용자 입력 대기 중 트랜잭션을 열어두지 않는다.

### `OFFSET` 기반 깊은 페이지네이션
앞의 모든 행을 읽는다. keyset 페이징을 쓴다.

## 4. 마이그레이션 안전 수칙

```sql
-- 항상 트랜잭션으로 (PostgreSQL은 DDL 롤백 가능)
BEGIN;
SET lock_timeout = '3s';
SET statement_timeout = '60s';
ALTER TABLE ... ;
COMMIT;
```

### 무중단 변경 순서
| 작업 | 안전한 방법 |
|---|---|
| 컬럼 추가 | `ADD COLUMN ... DEFAULT`는 PG11+ 즉시 완료. NOT NULL은 뒤에 |
| NOT NULL 추가 | `CHECK (c IS NOT NULL) NOT VALID` → `VALIDATE` → `SET NOT NULL` |
| 컬럼 삭제 | 애플리케이션에서 참조 제거 배포 → 이후 DROP |
| 컬럼명 변경 | 새 컬럼 추가 → 양쪽 쓰기 → 백필 → 읽기 전환 → 구 컬럼 삭제 |
| 타입 변경 | 신규 컬럼 방식 (직접 ALTER TYPE은 전체 재작성 + 락) |
| 인덱스 생성 | `CREATE INDEX CONCURRENTLY` (트랜잭션 밖에서) |
| FK 추가 | `NOT VALID` 로 추가 → `VALIDATE CONSTRAINT` |
| 대량 삭제 | 파티션 `DETACH` 또는 배치 DELETE |

```sql
-- NOT NULL 무중단 추가 예
ALTER TABLE t ADD CONSTRAINT ck_c_nn CHECK (c IS NOT NULL) NOT VALID;
ALTER TABLE t VALIDATE CONSTRAINT ck_c_nn;   -- 풀스캔이지만 약한 락
ALTER TABLE t ALTER COLUMN c SET NOT NULL;   -- PG12+ 는 위 CHECK 를 근거로 즉시 완료
ALTER TABLE t DROP CONSTRAINT ck_c_nn;
```

## 5. 스키마 설계 기본형

```sql
CREATE TABLE app.orders (
    id          bigint GENERATED ALWAYS AS IDENTITY,
    user_id     bigint      NOT NULL,
    order_no    text        NOT NULL,
    status      text        NOT NULL DEFAULT 'pending',
    amount      numeric(15,2) NOT NULL,
    meta        jsonb       NOT NULL DEFAULT '{}'::jsonb,
    created_at  timestamptz NOT NULL DEFAULT now(),
    updated_at  timestamptz NOT NULL DEFAULT now(),
    deleted_at  timestamptz,

    CONSTRAINT pk_orders        PRIMARY KEY (id),
    CONSTRAINT uk_orders_no     UNIQUE (order_no),
    CONSTRAINT fk_orders_users  FOREIGN KEY (user_id) REFERENCES app.users(id),
    CONSTRAINT ck_orders_amount CHECK (amount >= 0),
    CONSTRAINT ck_orders_status CHECK (status IN ('pending','paid','shipped','done','canceled'))
);

CREATE INDEX idx_orders_user_created ON app.orders (user_id, created_at DESC);
CREATE INDEX idx_orders_pending      ON app.orders (created_at) WHERE status = 'pending';

COMMENT ON TABLE  app.orders IS '주문';
COMMENT ON COLUMN app.orders.status IS '주문 상태';
```

- **NOT NULL을 기본으로**, 정말 없을 수 있는 값만 nullable
- **DEFAULT를 적극 활용** (`{}`, `now()`, `0`)
- **CHECK로 도메인 제약 표현** (상태값은 CHECK 또는 코드 테이블 FK)
- **소프트 삭제는 `deleted_at timestamptz`** + 부분 인덱스
- **모든 테이블에 `created_at`/`updated_at`**

## 6. 코드 리뷰 체크리스트

- [ ] 큰따옴표 식별자를 쓰지 않았는가
- [ ] 예약어를 이름으로 쓰지 않았는가
- [ ] `SELECT *`를 프로덕션 쿼리에 쓰지 않았는가
- [ ] `NOT IN` 서브쿼리에 NULL 가능성은 없는가
- [ ] 금액에 `numeric`을 썼는가
- [ ] 시각에 `timestamptz`를 썼는가
- [ ] INSERT에 컬럼명을 명시했는가
- [ ] 동적 SQL에 `USING`/`%I`/`%L`을 썼는가
- [ ] `SECURITY DEFINER`에 `search_path`를 지정했는가
- [ ] 함수 volatility(`IMMUTABLE`/`STABLE`)를 올바르게 선언했는가
- [ ] 새 인덱스는 `CONCURRENTLY`로 만드는가
- [ ] DDL 앞에 `lock_timeout`을 설정했는가
- [ ] 대량 DML을 배치로 나눴는가
- [ ] `EXPLAIN (ANALYZE, BUFFERS)`로 확인했는가
- [ ] 트랜잭션 범위가 필요 이상으로 길지 않은가
- [ ] PL/pgSQL 변수에 `v_`/`p_` 접두를 붙였는가
- [ ] `RETURNS TABLE` 함수에서 테이블 별칭을 붙였는가

## 관련 문서

- [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]] — 스키마 정의 문법
- [[PostgreSQL/01-BASICS|01. 기본 문법과 데이터 타입]] — 식별자 대소문자 규칙
- [[PostgreSQL/11-PERFORMANCE|11. 성능 — EXPLAIN, 인덱스 전략, 통계]] — 안티패턴이 성능에 미치는 영향
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
