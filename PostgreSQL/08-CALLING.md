---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - call
  - function-call
  - procedure
  - refcursor
  - jdbc
  - status/verified
aliases:
  - CALL
  - 함수 호출
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/07-PLPGSQL|07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)]]  ·  **다음** [[PostgreSQL/09-POSTGRES-ONLY|09. PostgreSQL 고유 기능]]

# 08. 프로시저 / 함수 호출 방법

## 1. 핵심 규칙

| 대상 | 호출 방법 |
|---|---|
| FUNCTION (스칼라 반환) | `SELECT fn(...);` |
| FUNCTION (SETOF / TABLE 반환) | `SELECT * FROM fn(...);` |
| PROCEDURE | `CALL proc(...);` |

**`CALL`로 함수를 부를 수 없고, `SELECT`로 프로시저를 부를 수 없다.**
Oracle에서 이관할 때 가장 많이 겪는 에러다.

```sql
SELECT fn_calc(100);        -- FUNCTION
CALL sp_batch(100);         -- PROCEDURE
CALL fn_calc(100);          -- ERROR: fn_calc(integer) is not a procedure
SELECT sp_batch(100);       -- ERROR: sp_batch(integer) is a procedure
```

## 2. 스칼라 함수 호출

```sql
-- 단독 호출
SELECT fn_calc_fee(10000);
SELECT fn_calc_fee(10000) AS fee;

-- 스키마 명시 (권장)
SELECT app.fn_calc_fee(10000);

-- SELECT 목록에서
SELECT id, amount, fn_calc_fee(amount) AS fee FROM orders;

-- WHERE 절에서
SELECT * FROM orders WHERE fn_calc_fee(amount) > 1000;

-- ORDER BY / GROUP BY 에서
SELECT fn_grade(score), count(*) FROM t GROUP BY fn_grade(score);

-- 다른 함수 인자로 중첩
SELECT coalesce(fn_calc_fee(amount), 0) FROM orders;

-- UPDATE / INSERT 에서
UPDATE orders SET fee = fn_calc_fee(amount);
INSERT INTO t (fee) VALUES (fn_calc_fee(10000));
```

### 인자 전달 방식
```sql
-- 위치 지정
SELECT fn_calc_fee(10000, 0.15);

-- 이름 지정 (=> 사용, PG9.5+. := 도 동작하나 => 권장)
SELECT fn_calc_fee(p_amount => 10000, p_rate => 0.15);
SELECT fn_calc_fee(p_rate => 0.15, p_amount => 10000);   -- 순서 무관

-- 혼합 (위치 인자가 먼저 와야 함)
SELECT fn_calc_fee(10000, p_rate => 0.15);
SELECT fn_calc_fee(p_amount => 10000, 0.15);   -- 에러

-- DEFAULT 생략
SELECT fn_calc_fee(10000);              -- p_rate 는 기본값 사용
-- 중간 파라미터만 생략하려면 이름 지정 필수
SELECT fn_x(a => 1, c => 3);            -- b는 기본값
```

### VARIADIC 호출
```sql
SELECT fn_sum(1, 2, 3, 4);
SELECT fn_sum(VARIADIC ARRAY[1,2,3,4]);   -- 배열을 펼쳐서 전달
```

## 3. 집합 반환 함수 호출

```sql
-- FROM 절에서 테이블처럼
SELECT * FROM fn_user_orders(1);
SELECT order_no, amount FROM fn_user_orders(1) WHERE amount > 1000;

-- 별칭 부여
SELECT o.* FROM fn_user_orders(1) AS o;
SELECT * FROM fn_user_orders(1) AS o(id, no, amt);   -- 컬럼명 재정의

-- 조인
SELECT u.name, o.order_no
FROM users u
CROSS JOIN LATERAL fn_user_orders(u.id) o;
-- 함수 호출은 LATERAL 키워드 생략 가능
SELECT u.name, o.order_no FROM users u, fn_user_orders(u.id) o;

-- SELECT 목록에서 호출 (행 확장. PG10+ 부터 동작 정리됨. 지양)
SELECT fn_user_orders(1);      -- 복합 타입 하나의 컬럼으로 나옴
SELECT (fn_user_orders(1)).*;  -- 펼쳐지지만 함수가 여러 번 실행될 수 있음

-- WITH ORDINALITY (행 번호 부여)
SELECT * FROM unnest(ARRAY['a','b','c']) WITH ORDINALITY AS t(val, idx);
```

### RETURNS record 함수 호출 (컬럼 정의 필요)
```sql
CREATE FUNCTION fn_rec() RETURNS record LANGUAGE plpgsql AS $$ ... $$;

SELECT * FROM fn_rec() AS t(a int, b text);   -- 반드시 타입 명시
```
→ 불편하므로 `RETURNS TABLE(...)`을 쓰는 것이 낫다.

## 4. OUT 파라미터 함수 호출

```sql
CREATE FUNCTION fn_div(IN a int, IN b int, OUT q int, OUT r int) ...;

SELECT * FROM fn_div(7, 3);         -- q | r  두 컬럼
SELECT fn_div(7, 3);                -- (2,1)  복합 값 한 컬럼
SELECT (fn_div(7,3)).q;             -- 필드 접근
SELECT q, r FROM fn_div(7, 3);
```

## 5. PROCEDURE 호출

```sql
CALL sp_batch_process();
CALL app.sp_batch_process(500);
CALL sp_batch_process(p_batch_size => 500);

-- INOUT 파라미터가 있으면 결과가 반환된다
CREATE PROCEDURE sp_x(INOUT p_cnt int) ...;
CALL sp_x(0);          -- 결과 세트로 p_cnt 값이 나옴
```

### PL/pgSQL 안에서 프로시저 호출
```sql
DO $$
DECLARE v_cnt int := 0;
BEGIN
    CALL sp_batch_process(500, v_cnt);   -- INOUT 은 변수를 넘겨야 값 회수 가능
    RAISE NOTICE '처리: %', v_cnt;
END $$;
```

**INOUT 인자에는 리터럴이 아닌 변수를 전달해야 값을 돌려받을 수 있다.**

### 트랜잭션 제어가 있는 프로시저
```sql
-- 자동 커밋 상태에서 호출해야 내부 COMMIT 이 동작
CALL sp_batch_process();

-- 명시적 트랜잭션 안에서는 실패
BEGIN;
CALL sp_batch_process();   -- ERROR: invalid transaction termination
COMMIT;
```
psql은 기본이 자동 커밋이라 그냥 `CALL`하면 되고, JDBC 등은
`setAutoCommit(true)`로 두어야 한다.

## 6. PL/pgSQL 내부에서 함수 호출

```sql
DECLARE
    v_fee numeric;
    v_row record;
BEGIN
    -- 스칼라
    v_fee := fn_calc_fee(10000);
    SELECT fn_calc_fee(10000) INTO v_fee;

    -- 집합 반환
    FOR v_row IN SELECT * FROM fn_user_orders(1) LOOP
        ...
    END LOOP;

    -- 반환값 무시 (PERFORM — SELECT 를 결과 없이 실행)
    PERFORM fn_side_effect(1);
    PERFORM 1 FROM t WHERE id = 1;   -- 존재 확인 (FOUND 로 판정)
    IF FOUND THEN ... END IF;

    -- 프로시저
    CALL sp_x(v_cnt);
END;
```

**PL/pgSQL 안에서 `SELECT fn();`만 쓰면 에러**(결과를 받을 곳이 없음).
`PERFORM`을 쓰거나 `INTO`로 받아야 한다.

## 7. 애플리케이션에서 호출

### JDBC (Java)
```java
// 스칼라 함수
try (PreparedStatement ps = conn.prepareStatement("SELECT app.fn_calc_fee(?)")) {
    ps.setBigDecimal(1, amount);
    ResultSet rs = ps.executeQuery();
    if (rs.next()) fee = rs.getBigDecimal(1);
}

// 집합 반환 함수
PreparedStatement ps = conn.prepareStatement("SELECT * FROM app.fn_user_orders(?)");

// 프로시저 — 반드시 CALL 사용, 자동 커밋 필요
conn.setAutoCommit(true);
try (CallableStatement cs = conn.prepareCall("CALL app.sp_batch_process(?)")) {
    cs.setInt(1, 500);
    cs.execute();
}

// INOUT 파라미터
CallableStatement cs = conn.prepareCall("CALL app.sp_x(?)");
cs.setInt(1, 0);
cs.registerOutParameter(1, Types.INTEGER);
cs.execute();
int result = cs.getInt(1);
```

**주의:** JDBC의 `{call fn(?)}` 이스케이프 문법은 PostgreSQL에서 함수/프로시저를
혼동시키기 쉽다. `SELECT`/`CALL`을 직접 쓰는 편이 안전하다.

### MyBatis
```xml
<!-- 함수 -->
<select id="calcFee" resultType="java.math.BigDecimal">
    SELECT app.fn_calc_fee(#{amount})
</select>

<select id="userOrders" resultType="OrderVO">
    SELECT * FROM app.fn_user_orders(#{userId})
</select>

<!-- 프로시저 -->
<update id="batch" statementType="CALLABLE">
    CALL app.sp_batch_process(#{size, mode=IN, jdbcType=INTEGER})
</update>

<select id="batchWithOut" statementType="CALLABLE">
    CALL app.sp_x(#{cnt, mode=INOUT, jdbcType=INTEGER})
</select>
```

### Python (psycopg 3)
```python
# 함수
cur.execute("SELECT app.fn_calc_fee(%s)", (10000,))
fee = cur.fetchone()[0]

cur.execute("SELECT * FROM app.fn_user_orders(%s)", (1,))
rows = cur.fetchall()

# 프로시저
conn.autocommit = True
cur.execute("CALL app.sp_batch_process(%s)", (500,))

# 구식 callproc() 은 psycopg3 에서 제거됨. CALL/SELECT 직접 사용
```

### Node.js (pg)
```js
const { rows } = await client.query('SELECT app.fn_calc_fee($1) AS fee', [10000]);
const res = await client.query('SELECT * FROM app.fn_user_orders($1)', [1]);
await client.query('CALL app.sp_batch_process($1)', [500]);
```

### psql
```sql
\df app.*                 -- 함수/프로시저 목록
\df+ app.fn_calc_fee      -- 상세 (소스 포함)
\sf app.fn_calc_fee       -- 정의 출력
\sf+ app.fn_calc_fee      -- 행 번호 포함

SELECT app.fn_calc_fee(10000);
CALL app.sp_batch_process(500);

-- NOTICE 메시지 레벨 조정
SET client_min_messages = 'notice';   -- RAISE NOTICE 보이게
SET client_min_messages = 'warning';  -- NOTICE 숨김
```

## 8. refcursor 반환 (Oracle SYS_REFCURSOR 대응)

```sql
CREATE FUNCTION fn_open_cur(p_id int) RETURNS refcursor
LANGUAGE plpgsql AS $$
DECLARE c refcursor := 'mycursor';
BEGIN
    OPEN c FOR SELECT * FROM t WHERE id = p_id;
    RETURN c;
END; $$;
```

```sql
-- 반드시 트랜잭션 안에서 (커밋 시 커서 닫힘)
BEGIN;
SELECT fn_open_cur(1);       -- 'mycursor' 반환
FETCH ALL FROM mycursor;
CLOSE mycursor;
COMMIT;
```

**PostgreSQL에서는 refcursor 대신 `RETURNS TABLE`을 쓰는 것이 정석이다.**
클라이언트 코드가 단순해지고 트랜잭션 관리가 필요 없다.

## 9. 함수 시그니처 해석 규칙

```sql
-- 오버로딩 시 타입에 따라 선택됨
CREATE FUNCTION f(int)  RETURNS text AS $$ SELECT 'int'  $$ LANGUAGE sql;
CREATE FUNCTION f(text) RETURNS text AS $$ SELECT 'text' $$ LANGUAGE sql;

SELECT f(1);          -- 'int'
SELECT f('a');        -- 'text'
SELECT f('1');        -- 모호할 수 있음 → 명시 캐스팅 권장
SELECT f('1'::int);   -- 'int'

-- 모호성 에러가 나면 캐스팅으로 해결
SELECT f(NULL::int);
```

**규칙: 오버로딩은 최소화한다.** 특히 `int`/`bigint`/`numeric` 오버로드는
암묵 캐스팅과 얽혀 예측하기 어렵다.

## 10. 흔한 호출 에러와 원인

| 에러 메시지 | 원인 |
|---|---|
| `function fn(...) does not exist` | 인자 타입 불일치, search_path 문제 → 스키마 명시 + 캐스팅 |
| `is not a procedure` | 함수를 CALL로 호출 |
| `is a procedure` | 프로시저를 SELECT로 호출 |
| `set-returning functions are not allowed in WHERE` | SETOF 함수를 WHERE에서 호출 → FROM 절로 이동 |
| `query has no destination for result data` | PL/pgSQL 안에서 `SELECT fn()` → `PERFORM` 사용 |
| `a column definition list is required` | `RETURNS record` 함수 → `AS t(a int, b text)` 추가 |
| `invalid transaction termination` | 트랜잭션 블록 안에서 프로시저의 COMMIT |
| `function is not unique` | 오버로딩 모호 → 인자 캐스팅 |

## 관련 문서

- [[PostgreSQL/07-PLPGSQL|07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)]] — 호출 대상 FUNCTION/PROCEDURE 작성법
- [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]] — 내장 함수 호출 형태
- [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]] — Oracle SYS_REFCURSOR 대응
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
