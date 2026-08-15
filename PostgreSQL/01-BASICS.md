---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - basics
  - identifier
  - data-type
  - null
  - status/verified
aliases:
  - PostgreSQL 식별자 규칙
  - 데이터 타입
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **다음** [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]]

# 01. 기본 문법과 데이터 타입

## 1. 식별자 (Identifier) 규칙

### 대소문자 처리 — PostgreSQL의 함정
PostgreSQL은 **따옴표 없는 식별자를 소문자로 변환**한다. (Oracle은 대문자로 변환 → 정반대)

```sql
CREATE TABLE MyTable (UserId int);   -- 실제로는 mytable, userid 로 저장됨
SELECT * FROM MYTABLE;               -- OK (mytable로 변환되어 매칭)
SELECT * FROM "MyTable";             -- 에러! "MyTable" 이라는 테이블은 없음

CREATE TABLE "MyTable" ("UserId" int);  -- 대소문자 그대로 저장
SELECT * FROM "MyTable";                -- 반드시 따옴표 필요
SELECT * FROM MyTable;                  -- 에러!
```

**규칙: 큰따옴표는 쓰지 않는다.** 전부 `snake_case` 소문자로 통일하면 인용부호 지옥을 피할 수 있다.

### 식별자 제약
- 최대 63 바이트 (`NAMEDATALEN - 1`). 초과분은 **경고 없이 잘림**
- 시작 문자: 문자 또는 `_` (숫자 불가)
- 허용 문자: 문자, 숫자, `_`, `$`
- 예약어를 쓰려면 큰따옴표 필요 → 애초에 피할 것

```sql
-- 예약어 확인
SELECT * FROM pg_get_keywords() WHERE catcode = 'R';  -- R = reserved
```

## 2. 문자열 리터럴

```sql
-- 표준 작은따옴표. 내부 작은따옴표는 두 번
SELECT 'It''s ok';

-- E'' : 이스케이프 문자열 (C 스타일)
SELECT E'line1\nline2\ttab';

-- $$ ... $$ : 달러 인용 (Dollar Quoting) — PostgreSQL 고유
-- 이스케이프가 전혀 필요 없음. 함수 본문에 필수적으로 사용
SELECT $$It's a "quoted" string with \n literal$$;
SELECT $tag$중첩 시 태그 사용 $$ 포함 가능$tag$;

-- U&'' : 유니코드 이스케이프
SELECT U&'\D55C\AD6D';   -- 한국

-- 문자열 연속 (줄바꿈으로 자동 연결)
SELECT 'foo'
       'bar';   -- 'foobar'
```

### 비트/바이너리 리터럴
```sql
SELECT B'1010';        -- bit string
SELECT X'1FF';         -- hex bit string
SELECT '\x48656c6c6f'::bytea;  -- bytea hex 포맷
```

## 3. 주석

```sql
-- 한 줄 주석

/* 여러 줄 주석
   /* 중첩 가능 (표준 SQL과 달리 PostgreSQL은 중첩 지원) */
*/

-- 객체에 주석 달기 (메타데이터로 저장됨)
COMMENT ON TABLE users IS '사용자 마스터';
COMMENT ON COLUMN users.email IS '로그인 ID로 사용';
COMMENT ON FUNCTION fn_calc(int) IS '요금 계산';

-- 주석 조회
SELECT obj_description('users'::regclass, 'pg_class');
SELECT col_description('users'::regclass, 1);
```

## 4. 캐스팅 (형변환)

```sql
CAST('123' AS integer)   -- 표준 SQL
'123'::integer           -- PostgreSQL 고유 문법 (권장, 간결)
integer '123'            -- 타입 이름 접두 (리터럴만 가능)

-- 배열/복합 타입도 동일
'{1,2,3}'::int[]
'2026-08-03'::date
'{"a":1}'::jsonb
```

**주의:** `::`는 연산자 우선순위가 매우 높다.
```sql
SELECT -1::int;      -- -(1::int) = -1
SELECT (1+2)::text;  -- 괄호 필요
```

## 5. 데이터 타입

### 숫자
| 타입 | 크기 | 범위/설명 |
|---|---|---|
| `smallint` (int2) | 2B | -32,768 ~ 32,767 |
| `integer` (int4, int) | 4B | ±21억 |
| `bigint` (int8) | 8B | ±922경 |
| `numeric(p,s)` / `decimal` | 가변 | **정확한 소수. 금액에 필수** |
| `real` (float4) | 4B | 부동소수점 6자리 |
| `double precision` (float8) | 8B | 부동소수점 15자리 |
| `smallserial`/`serial`/`bigserial` | - | 자동 증가 (레거시, IDENTITY 권장) |

```sql
-- 금액은 반드시 numeric
SELECT 0.1::float8 + 0.2::float8;    -- 0.30000000000000004
SELECT 0.1::numeric + 0.2::numeric;  -- 0.3

-- numeric 특수값
SELECT 'NaN'::numeric;      -- NaN (numeric에서는 NaN = NaN 이 참)
SELECT 'Infinity'::float8;

-- 정수 나눗셈 주의 (Oracle과 다름)
SELECT 7 / 2;          -- 3  (정수 나눗셈)
SELECT 7 / 2.0;        -- 3.5
SELECT 7::numeric / 2; -- 3.5
```

### 문자열
| 타입 | 설명 |
|---|---|
| `text` | 길이 무제한. **PostgreSQL에서는 이게 기본** |
| `varchar(n)` | n자 제한. 초과 시 에러 |
| `char(n)` | 고정 길이, 공백 패딩. **쓰지 말 것** |

**중요:** PostgreSQL에서 `text`와 `varchar`는 **성능 차이가 없다**. 길이 제약이 비즈니스 규칙일 때만 `varchar(n)`을 쓰고, 아니면 `text`.

```sql
-- char(n)의 함정: 공백 패딩 후 비교 시 무시됨
SELECT 'a '::char(5) = 'a'::char(5);  -- true
SELECT length('a'::char(5));          -- 1 (패딩 무시)
SELECT length('a'::char(5)::text);    -- 5 (패딩 보임)
```

### 날짜/시간
| 타입 | 설명 |
|---|---|
| `date` | 날짜만 |
| `time [without time zone]` | 시간만 |
| `timetz` | 시간+타임존 (**쓰지 말 것**, 의미 모호) |
| `timestamp` | 날짜+시간, 타임존 없음 |
| `timestamptz` | 날짜+시간, **타임존 인식. 기본 선택** |
| `interval` | 기간 |

```sql
-- timestamptz는 내부적으로 UTC 저장, 조회 시 세션 TimeZone으로 변환
SET TimeZone = 'Asia/Seoul';
SELECT now();                          -- 2026-08-03 09:00:00+09
SELECT now() AT TIME ZONE 'UTC';       -- 2026-08-03 00:00:00 (timestamp)

-- 현재 시각 함수의 차이
SELECT now();                -- 트랜잭션 시작 시각 (고정)
SELECT current_timestamp;    -- now()와 동일
SELECT statement_timestamp();-- 현재 statement 시작 시각
SELECT clock_timestamp();    -- 실제 호출 순간 (매번 다름)

-- interval
SELECT now() + interval '1 day 3 hours';
SELECT now() - '2026-01-01'::timestamptz;   -- interval 반환
SELECT interval '1 month';   -- 주의: 월은 가변 길이

-- 무한대 지원
SELECT 'infinity'::timestamptz, '-infinity'::date;
```

### 불리언
```sql
-- TRUE 로 인식: TRUE, 't', 'true', 'y', 'yes', 'on', '1'
-- FALSE 로 인식: FALSE, 'f', 'false', 'n', 'no', 'off', '0'
SELECT 'y'::boolean, 1::boolean, 0::boolean;

-- NULL 삼값 논리
SELECT true AND NULL;    -- NULL
SELECT false AND NULL;   -- false
SELECT true OR NULL;     -- true
SELECT NULL = NULL;      -- NULL (false 아님!)
SELECT NULL IS NULL;     -- true
SELECT 1 IS DISTINCT FROM NULL;  -- true (NULL 안전 비교)
```

### 기타 주요 타입
```sql
uuid           -- gen_random_uuid() 로 생성 (PG13+ 내장)
json / jsonb   -- jsonb 권장 (09번 문서 참조)
int[] / text[] -- 배열 (09번 문서)
int4range, tsrange, daterange  -- 범위 타입
inet, cidr, macaddr            -- 네트워크
point, line, polygon, circle   -- 기하
tsvector, tsquery              -- 전문 검색
xml
bytea          -- 바이너리
enum           -- CREATE TYPE ... AS ENUM
```

## 6. NULL 처리

```sql
-- NULL 은 값이 아니라 '알 수 없음'
SELECT COALESCE(col, '기본값');           -- 첫 non-NULL 반환
SELECT NULLIF(col, '');                  -- 같으면 NULL
SELECT GREATEST(1, NULL, 3);             -- 3 (NULL 무시)

-- NULL 안전 비교
a IS DISTINCT FROM b       -- NULL 포함 비교, 다르면 true
a IS NOT DISTINCT FROM b   -- NULL 포함 비교, 같으면 true

-- 정렬 시 NULL 위치 (기본: ASC면 마지막, DESC면 처음)
ORDER BY col ASC NULLS FIRST
ORDER BY col DESC NULLS LAST

-- 집계 함수는 NULL 무시
SELECT COUNT(*), COUNT(col) FROM t;  -- count(col)은 NULL 제외
SELECT AVG(col) FROM t;              -- NULL 제외하고 평균
```

## 7. 연산자

```sql
-- 문자열
'a' || 'b'                -- 연결. 한쪽이라도 NULL이면 NULL
concat('a', NULL, 'b')    -- 'ab' (NULL 무시)

-- 패턴 매칭
LIKE / NOT LIKE           -- % _ 와일드카드
ILIKE                     -- 대소문자 무시 LIKE (PostgreSQL 고유)
SIMILAR TO                -- SQL 표준 정규식 (거의 안 씀)
~   / !~                  -- POSIX 정규식 (대소문자 구분)
~*  / !~*                 -- POSIX 정규식 (대소문자 무시)

SELECT 'Hello' ILIKE 'hello';   -- true
SELECT 'abc123' ~ '^[a-z]+\d+$'; -- true

-- 산술
5 % 3      -- 나머지 2
2 ^ 10     -- 거듭제곱 1024
|/ 25      -- 제곱근 5
@ -5       -- 절대값 5
```

## 8. 스키마와 search_path

```sql
CREATE SCHEMA app;
SET search_path = app, public;   -- 세션 단위
SHOW search_path;

-- 검색 경로 없이 명시적 참조 (권장)
SELECT * FROM app.users;

-- 함수 내부에서는 search_path 고정 권장 (보안: search_path 주입 방지)
CREATE FUNCTION f() RETURNS int LANGUAGE plpgsql
SECURITY DEFINER SET search_path = pg_catalog, app AS $$ ... $$;
```

## 관련 문서

- [[PostgreSQL/02-DDL|02. DDL — 테이블, 제약조건, 인덱스]] — 식별자·타입을 실제 테이블 정의에 적용
- [[PostgreSQL/09-POSTGRES-ONLY|09. PostgreSQL 고유 기능]] — 배열·JSONB 등 고유 타입 상세
- [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]] — 대소문자 처리가 Oracle과 정반대인 지점
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
