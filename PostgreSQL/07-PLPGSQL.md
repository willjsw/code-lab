---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - plpgsql
  - function
  - procedure
  - trigger
  - cursor
  - status/verified
aliases:
  - PL/pgSQL
  - 저장 프로시저
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/06-WINDOW|06. 윈도우 함수]]  ·  **다음** [[PostgreSQL/08-CALLING|08. 프로시저 / 함수 호출 방법]]

# 07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)

## 1. FUNCTION vs PROCEDURE

| | FUNCTION | PROCEDURE (PG11+) |
|---|---|---|
| 정의 | `CREATE FUNCTION` | `CREATE PROCEDURE` |
| 호출 | `SELECT fn(...)` | `CALL proc(...)` |
| 반환 | `RETURNS ...` 필수 | 반환 없음 (`INOUT` 파라미터로 값 전달 가능) |
| SQL 내 사용 | 가능 (SELECT, WHERE 등) | 불가 |
| 트랜잭션 제어 | **불가** (COMMIT/ROLLBACK 못 씀) | **가능** |
| 용도 | 값 계산, 조회 | 배치 작업, 다단계 트랜잭션 |

**선택 기준:** 값을 반환하고 쿼리 안에서 써야 하면 FUNCTION. 중간 COMMIT이 필요한
배치 작업이면 PROCEDURE.

## 2. FUNCTION 기본 형태

```sql
CREATE OR REPLACE FUNCTION app.fn_calc_fee(
    p_amount  numeric,
    p_rate    numeric DEFAULT 0.1
)
RETURNS numeric
LANGUAGE plpgsql
IMMUTABLE
AS $$
DECLARE
    v_fee numeric;
BEGIN
    IF p_amount IS NULL OR p_amount < 0 THEN
        RAISE EXCEPTION '금액이 올바르지 않습니다: %', p_amount
            USING ERRCODE = 'invalid_parameter_value';
    END IF;

    v_fee := round(p_amount * p_rate, 0);
    RETURN v_fee;
END;
$$;
```

### 본문은 반드시 달러 인용($$)으로
작은따옴표로 감싸면 내부 문자열마다 이스케이프해야 한다. 관례적으로 `$$` 또는
`$function$`, `$body$` 태그를 쓴다.

## 3. 함수 속성 (Volatility) — 성능에 직결

| 속성 | 의미 | 사용 예 |
|---|---|---|
| `IMMUTABLE` | 같은 입력 → 항상 같은 출력. DB 접근 없음 | 순수 계산 |
| `STABLE` | 한 statement 내에서 일관된 결과. SELECT 가능 | 조회 함수 |
| `VOLATILE` (기본) | 매번 다를 수 있음. 부작용 있음 | INSERT/UPDATE 수행 |

```sql
-- 잘못된 IMMUTABLE 선언은 인덱스 손상/오답을 유발한다.
-- 테이블을 읽으면 IMMUTABLE 이 될 수 없다 (최소 STABLE).
CREATE FUNCTION f() RETURNS int LANGUAGE sql STABLE AS $$ SELECT count(*)::int FROM t $$;
```

### 기타 속성
```sql
STRICT                     -- 인자 중 NULL이 있으면 본문 실행 없이 NULL 반환
                           -- (= RETURNS NULL ON NULL INPUT)
CALLED ON NULL INPUT       -- 기본값
SECURITY DEFINER           -- 함수 소유자 권한으로 실행 (기본은 INVOKER)
SECURITY INVOKER           -- 호출자 권한 (기본)
PARALLEL SAFE|RESTRICTED|UNSAFE   -- 병렬 쿼리 사용 가능 여부
COST 100                   -- 실행 비용 추정치
ROWS 1000                  -- 집합 반환 함수의 예상 행 수
LEAKPROOF                  -- RLS 하에서 조건 푸시다운 허용 (superuser만)
SET search_path = pg_catalog, app   -- 실행 시 GUC 고정
```

**보안 규칙:** `SECURITY DEFINER` 함수는 **반드시 `SET search_path`를 명시**한다.
안 하면 search_path 조작을 통한 권한 상승 공격이 가능하다.

```sql
CREATE FUNCTION admin_op() RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = pg_catalog, app
AS $$ BEGIN ... END; $$;
```

## 4. 파라미터

```sql
-- 모드: IN(기본) / OUT / INOUT / VARIADIC
CREATE FUNCTION fn_div(
    IN  p_a int,
    IN  p_b int,
    OUT o_quotient int,
    OUT o_remainder int
) LANGUAGE plpgsql AS $$
BEGIN
    o_quotient  := p_a / p_b;
    o_remainder := p_a % p_b;
END; $$;
-- OUT 파라미터만 여러 개면 RETURNS 생략 가능 (record 반환)
SELECT * FROM fn_div(7, 3);   -- o_quotient=2, o_remainder=1

-- 가변 인자
CREATE FUNCTION fn_sum(VARIADIC p_nums int[]) RETURNS int
LANGUAGE plpgsql AS $$
DECLARE v_total int := 0; v_n int;
BEGIN
    FOREACH v_n IN ARRAY p_nums LOOP v_total := v_total + v_n; END LOOP;
    RETURN v_total;
END; $$;
SELECT fn_sum(1,2,3,4);
SELECT fn_sum(VARIADIC ARRAY[1,2,3]);

-- 기본값 (뒤쪽 파라미터부터만 가능)
CREATE FUNCTION f(a int, b int DEFAULT 0, c text DEFAULT 'x') ...
```

## 5. 반환 타입

```sql
-- 스칼라
RETURNS integer / text / boolean / void

-- 테이블의 행 타입
RETURNS users
RETURNS SETOF users            -- 여러 행

-- 임의 컬럼 구성 (권장 - 가장 명확)
RETURNS TABLE (id bigint, name text, cnt int)

-- record (호출 시 컬럼 정의 필요 - 불편, 지양)
RETURNS record
RETURNS SETOF record
```

```sql
-- RETURNS TABLE 예시
CREATE OR REPLACE FUNCTION fn_user_orders(p_user_id bigint)
RETURNS TABLE (order_id bigint, order_no text, amount numeric)
LANGUAGE plpgsql STABLE
AS $$
BEGIN
    RETURN QUERY
        SELECT o.id, o.order_no, o.amount
        FROM orders o
        WHERE o.user_id = p_user_id
        ORDER BY o.created_at DESC;
END;
$$;

SELECT * FROM fn_user_orders(1);
```

**RETURNS TABLE 이름 충돌 주의:** 출력 컬럼명(`order_id` 등)이 변수처럼 동작하므로
테이블 컬럼과 이름이 같으면 모호성 에러가 난다. 위처럼 **테이블 별칭을 반드시 붙일 것**.

## 6. LANGUAGE sql — 간단하면 이쪽이 낫다

```sql
-- 단순 쿼리는 plpgsql 오버헤드 없이 SQL 함수로
CREATE OR REPLACE FUNCTION fn_active_count() RETURNS bigint
LANGUAGE sql STABLE
AS $$ SELECT count(*) FROM users WHERE status = 'active' $$;

-- PG14+ 표준 문법 (본문이 파싱 시점에 검증됨 → 오타를 배포 시 잡음)
CREATE OR REPLACE FUNCTION fn_active_count() RETURNS bigint
LANGUAGE sql STABLE
BEGIN ATOMIC
    SELECT count(*) FROM users WHERE status = 'active';
END;

-- SQL 함수는 인라인 될 수 있어 plpgsql 보다 빠른 경우가 많다
```

**규칙: 제어 흐름(IF/LOOP/예외처리)이 필요 없으면 `LANGUAGE sql`을 쓴다.**

## 7. PL/pgSQL 문법

### 블록 구조
```sql
DO $$                        -- 익명 블록 (일회성 실행)
DECLARE
    v_x int := 1;
BEGIN
    RAISE NOTICE 'x=%', v_x;
END $$;
```

### 변수 선언
```sql
DECLARE
    v_id        bigint;
    v_name      text := '기본값';
    v_cnt       int NOT NULL DEFAULT 0;
    c_max       constant int := 100;

    v_status    users.status%TYPE;      -- 컬럼 타입 참조
    v_row       users%ROWTYPE;          -- 행 전체 타입
    v_rec       record;                 -- 동적 레코드

    v_arr       int[];
    v_json      jsonb;
BEGIN
    ...
END;
```

### 대입
```sql
v_x := 1;
v_x = 1;                                  -- 동작하지만 := 권장

SELECT col INTO v_x FROM t WHERE id = 1;  -- 없으면 NULL, 여러 행이면 첫 행
SELECT col INTO STRICT v_x FROM t WHERE id = 1;
-- STRICT: 정확히 1행 아니면 예외 (NO_DATA_FOUND / TOO_MANY_ROWS)

SELECT a, b INTO v_a, v_b FROM t WHERE id = 1;
SELECT * INTO v_row FROM users WHERE id = 1;

INSERT INTO t (a) VALUES (1) RETURNING id INTO v_id;
UPDATE t SET a=1 WHERE id=2 RETURNING * INTO v_row;
```

### 조건문
```sql
IF cond THEN
    ...
ELSIF cond2 THEN     -- ELSIF (ELSEIF도 허용, ELSE IF는 다른 의미)
    ...
ELSE
    ...
END IF;

CASE v_x
    WHEN 1, 2 THEN ...
    WHEN 3    THEN ...
    ELSE ...           -- ELSE 없고 매칭 실패 시 CASE_NOT_FOUND 예외
END CASE;

CASE
    WHEN v_x > 10 THEN ...
    ELSE ...
END CASE;
```

### 반복문
```sql
-- 무한 루프
LOOP
    EXIT WHEN cond;
    CONTINUE WHEN cond2;
END LOOP;

WHILE cond LOOP ... END LOOP;

FOR i IN 1..10 LOOP ... END LOOP;
FOR i IN REVERSE 10..1 LOOP ... END LOOP;
FOR i IN 1..10 BY 2 LOOP ... END LOOP;

-- 쿼리 결과 순회
FOR v_rec IN SELECT * FROM users WHERE status='active' LOOP
    RAISE NOTICE '%', v_rec.name;
END LOOP;

-- 동적 쿼리 순회
FOR v_rec IN EXECUTE format('SELECT * FROM %I', v_table) LOOP ... END LOOP;

-- 배열 순회
FOREACH v_n IN ARRAY v_arr LOOP ... END LOOP;
FOREACH v_slice SLICE 1 IN ARRAY v_2d LOOP ... END LOOP;

-- 라벨 (중첩 루프 탈출)
<<outer>>
FOR i IN 1..10 LOOP
    FOR j IN 1..10 LOOP
        EXIT outer WHEN i*j > 50;
    END LOOP;
END LOOP outer;
```

**성능 주의:** 행 단위 루프는 셋 기반 SQL보다 압도적으로 느리다.
루프를 쓰기 전에 단일 UPDATE/INSERT ... SELECT 로 해결되는지 먼저 검토할 것.

### 반환문
```sql
RETURN v_x;                    -- 스칼라 반환, 즉시 종료
RETURN;                        -- void 또는 SETOF 종료
RETURN NEXT v_row;             -- SETOF: 행 하나씩 누적 (전체 누적 후 반환)
RETURN QUERY SELECT ...;       -- SETOF: 쿼리 결과 통째로 누적
RETURN QUERY EXECUTE format(...) USING v_a;
```

```sql
CREATE FUNCTION fn_rows() RETURNS SETOF users LANGUAGE plpgsql AS $$
DECLARE v_row users%ROWTYPE;
BEGIN
    FOR v_row IN SELECT * FROM users LOOP
        v_row.name := upper(v_row.name);
        RETURN NEXT v_row;       -- 여기서 끝나지 않고 계속 진행
    END LOOP;
    RETURN;                      -- 명시적 종료
END; $$;
```

### 특수 변수
```sql
FOUND        -- 직전 SELECT INTO / UPDATE / DELETE / FOR 루프가 행을 처리했는지 (boolean)
ROW_COUNT    -- GET DIAGNOSTICS 로 조회

UPDATE t SET a=1 WHERE id=999;
IF NOT FOUND THEN RAISE EXCEPTION '대상 없음'; END IF;

GET DIAGNOSTICS v_cnt = ROW_COUNT;
GET DIAGNOSTICS v_ctx = PG_CONTEXT;
```

## 8. 예외 처리

```sql
BEGIN
    ...
EXCEPTION
    WHEN unique_violation THEN
        RAISE NOTICE '중복';
    WHEN foreign_key_violation OR check_violation THEN
        ...
    WHEN division_by_zero THEN
        RETURN 0;
    WHEN OTHERS THEN
        GET STACKED DIAGNOSTICS
            v_msg    = MESSAGE_TEXT,
            v_detail = PG_EXCEPTION_DETAIL,
            v_hint   = PG_EXCEPTION_HINT,
            v_state  = RETURNED_SQLSTATE,
            v_ctx    = PG_EXCEPTION_CONTEXT;
        RAISE WARNING '실패: % (%)', v_msg, v_state;
        RAISE;                    -- 원본 예외 재발생
END;
```

**중요: `EXCEPTION` 블록은 내부적으로 SAVEPOINT를 생성한다.**
루프 안에서 매 회 예외 블록을 쓰면 성능이 크게 떨어진다. 꼭 필요한 곳에만 쓸 것.

### 자주 쓰는 예외 코드
| 이름 | SQLSTATE |
|---|---|
| `unique_violation` | 23505 |
| `foreign_key_violation` | 23503 |
| `not_null_violation` | 23502 |
| `check_violation` | 23514 |
| `no_data_found` | P0002 |
| `too_many_rows` | P0003 |
| `division_by_zero` | 22012 |
| `invalid_text_representation` | 22P02 |
| `deadlock_detected` | 40P01 |
| `serialization_failure` | 40001 |
| `raise_exception` | P0001 (기본 RAISE EXCEPTION) |

### RAISE
```sql
RAISE DEBUG   '...';
RAISE LOG     '...';
RAISE INFO    '...';
RAISE NOTICE  '값=%', v_x;      -- % 는 위치 치환. %% 는 리터럴 %
RAISE WARNING '...';
RAISE EXCEPTION '오류: %', v_x  -- 트랜잭션 중단
    USING ERRCODE = 'P0001',
          DETAIL  = '상세 내용',
          HINT    = '조치 방법';

RAISE EXCEPTION USING ERRCODE = 'unique_violation';
RAISE;   -- 예외 블록 안에서만: 원본 재발생
```

## 9. 동적 SQL

```sql
EXECUTE format('SELECT * FROM %I WHERE id = $1', v_table)
   INTO v_row
  USING v_id;

EXECUTE 'UPDATE t SET a = $1 WHERE id = $2' USING v_a, v_id;

-- format 지정자
-- %I : 식별자 (자동 큰따옴표, 인젝션 방어)
-- %L : 리터럴 (자동 작은따옴표+이스케이프, NULL은 NULL로)
-- %s : 문자열 그대로 (위험 — 사용자 입력에 절대 쓰지 말 것)
```

**규칙: 값은 `USING` 바인딩, 식별자는 `%I`.** 문자열 연결(`||`)로 SQL을 조립하면
인젝션 취약점이 된다.

```sql
-- 나쁨
EXECUTE 'SELECT * FROM t WHERE name = ''' || p_name || '''';
-- 좋음
EXECUTE 'SELECT * FROM t WHERE name = $1' USING p_name;
```

## 10. PROCEDURE

```sql
CREATE OR REPLACE PROCEDURE app.sp_batch_process(
    p_batch_size int DEFAULT 1000,
    INOUT p_processed int DEFAULT 0
)
LANGUAGE plpgsql
AS $$
DECLARE
    v_cnt int;
BEGIN
    LOOP
        UPDATE jobs SET status = 'done'
        WHERE id IN (SELECT id FROM jobs WHERE status='pending' LIMIT p_batch_size);

        GET DIAGNOSTICS v_cnt = ROW_COUNT;
        EXIT WHEN v_cnt = 0;

        p_processed := p_processed + v_cnt;
        COMMIT;                    -- 프로시저에서만 가능
        RAISE NOTICE '진행: %건', p_processed;
    END LOOP;
END;
$$;

CALL app.sp_batch_process(500);
```

**프로시저 트랜잭션 제어 제약**
- `CALL`이 이미 트랜잭션 블록(`BEGIN ... COMMIT`) 안에 있으면 `COMMIT`/`ROLLBACK` 불가
- 예외 처리 블록(`EXCEPTION`)이 있는 블록 안에서는 트랜잭션 제어 불가
- 커서가 열려 있으면(HOLD 아닌) 커밋 시 닫힘

```sql
-- COMMIT 대신 트랜잭션 체이닝
COMMIT AND CHAIN;    -- 커밋 후 같은 특성으로 새 트랜잭션 시작
ROLLBACK AND CHAIN;
```

## 11. 커서

```sql
DECLARE
    c_users CURSOR FOR SELECT * FROM users;
    c_param CURSOR (p_status text) FOR SELECT * FROM users WHERE status = p_status;
    c_ref   refcursor;
    v_row   record;
BEGIN
    OPEN c_users;
    LOOP
        FETCH c_users INTO v_row;
        EXIT WHEN NOT FOUND;
        ...
    END LOOP;
    CLOSE c_users;

    -- 동적 커서
    OPEN c_ref FOR EXECUTE format('SELECT * FROM %I', v_tbl);

    -- 위치 이동
    FETCH FIRST FROM c_ref INTO v_row;
    FETCH LAST  FROM c_ref INTO v_row;
    FETCH ABSOLUTE 5 FROM c_ref INTO v_row;
    MOVE FORWARD 10 FROM c_ref;

    -- 커서 위치 갱신/삭제
    UPDATE t SET a=1 WHERE CURRENT OF c_users;
END;
```

**대부분의 경우 `FOR ... IN SELECT` 루프가 커서보다 간결하고 빠르다.**
커서는 refcursor를 클라이언트로 반환하거나 위치 이동이 필요할 때만 쓴다.

## 12. 트리거 함수

```sql
CREATE OR REPLACE FUNCTION fn_set_updated_at()
RETURNS trigger
LANGUAGE plpgsql
AS $$
BEGIN
    NEW.updated_at := now();
    RETURN NEW;         -- BEFORE 트리거: NEW 반환해야 반영. NULL 반환 시 작업 취소
END;
$$;

CREATE TRIGGER trg_users_updated
    BEFORE UPDATE ON users
    FOR EACH ROW
    WHEN (OLD.* IS DISTINCT FROM NEW.*)    -- 실제 변경 있을 때만
    EXECUTE FUNCTION fn_set_updated_at();
```

### 트리거 특수 변수
| 변수 | 설명 |
|---|---|
| `NEW` | 새 행 (INSERT/UPDATE) |
| `OLD` | 이전 행 (UPDATE/DELETE) |
| `TG_OP` | 'INSERT'/'UPDATE'/'DELETE'/'TRUNCATE' |
| `TG_TABLE_NAME` / `TG_TABLE_SCHEMA` | 테이블 정보 |
| `TG_WHEN` | 'BEFORE'/'AFTER'/'INSTEAD OF' |
| `TG_LEVEL` | 'ROW'/'STATEMENT' |
| `TG_ARGV[]` / `TG_NARGS` | CREATE TRIGGER 인자 |

```sql
-- 트리거 옵션
BEFORE | AFTER | INSTEAD OF                 -- INSTEAD OF 는 뷰에만
INSERT OR UPDATE OF col1, col2 OR DELETE
FOR EACH ROW | FOR EACH STATEMENT
WHEN (조건)                                  -- BEFORE/AFTER 에서만

-- 전이 테이블 (PG10+, STATEMENT 트리거에서 변경 행 집합 접근)
CREATE TRIGGER trg AFTER UPDATE ON t
    REFERENCING OLD TABLE AS old_rows NEW TABLE AS new_rows
    FOR EACH STATEMENT EXECUTE FUNCTION fn();
-- 함수 안에서: SELECT * FROM new_rows;  ← 행 단위 트리거보다 훨씬 빠름

-- 트리거 비활성화 (대량 적재 시)
ALTER TABLE t DISABLE TRIGGER trg_x;
ALTER TABLE t ENABLE  TRIGGER ALL;
```

**반환값 규칙**
- `BEFORE ROW`: `NEW` 반환 → 진행 / `NULL` 반환 → 해당 행 작업 취소
- `AFTER ROW`, `STATEMENT`: 반환값 무시 (`NULL` 반환)
- `INSTEAD OF`: `NULL`이 아닌 값을 반환해야 처리된 것으로 간주

## 13. 함수 관리

```sql
-- 오버로딩 가능. 삭제 시 시그니처 명시
DROP FUNCTION IF EXISTS fn_calc(numeric, numeric);
DROP PROCEDURE IF EXISTS sp_batch(int);
DROP ROUTINE IF EXISTS fn_x(int);      -- PG11+ 함수/프로시저 공통

ALTER FUNCTION fn_x(int) RENAME TO fn_y;
ALTER FUNCTION fn_x(int) OWNER TO app_user;
ALTER FUNCTION fn_x(int) SET search_path = app;

-- 정의 조회
SELECT prosrc FROM pg_proc WHERE proname = 'fn_calc';
SELECT pg_get_functiondef('app.fn_calc(numeric,numeric)'::regprocedure);

-- 목록 조회
SELECT n.nspname, p.proname,
       pg_get_function_arguments(p.oid) AS args,
       pg_get_function_result(p.oid)    AS result,
       CASE p.prokind WHEN 'f' THEN 'function' WHEN 'p' THEN 'procedure' END AS kind
FROM pg_proc p JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE n.nspname = 'app'
ORDER BY 1, 2;
```

**`CREATE OR REPLACE FUNCTION`은 파라미터 이름/타입/반환타입을 바꿀 수 없다.**
바꾸려면 `DROP` 후 재생성해야 하며, 파라미터 개수가 다르면 오버로드로 별개 함수가 만들어져
기존 함수가 남아 있게 되니 주의.

## 관련 문서

- [[PostgreSQL/08-CALLING|08. 프로시저 / 함수 호출 방법]] — 작성한 FUNCTION/PROCEDURE 호출 방법
- [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]] — 내장 함수 — 직접 작성 전 존재 여부 확인
- [[PostgreSQL/10-TRANSACTION|10. 트랜잭션, 격리 수준, 락]] — PROCEDURE 내 COMMIT/ROLLBACK 제약
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
