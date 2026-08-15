---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - function
  - string-function
  - date-function
  - aggregate
  - status/verified
aliases:
  - 내장 함수
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/04-JOIN-SUBQUERY|04. JOIN, 서브쿼리, CTE]]  ·  **다음** [[PostgreSQL/06-WINDOW|06. 윈도우 함수]]

# 05. 내장 함수

## 1. 문자열 함수

```sql
length('abc')                    -- 3 (문자 수)
octet_length('한글')              -- 6 (바이트 수)
upper / lower / initcap('hello world')   -- HELLO WORLD / hello world / Hello World
trim(' x ') / ltrim / rtrim
trim(both '0' from '00abc00')    -- 'abc'
lpad('5', 3, '0')                -- '005'
rpad('5', 3, '0')                -- '500'

substring('abcdef', 2, 3)        -- 'bcd'  (1-based)
substring('abcdef' FROM 2 FOR 3) -- 표준 문법
substr('abcdef', 2, 3)           -- 동일
left('abcdef', 2) / right('abcdef', 2)   -- 'ab' / 'ef'

position('cd' IN 'abcdef')       -- 3 (없으면 0)
strpos('abcdef', 'cd')           -- 3
replace('a-b-c', '-', '_')       -- 'a_b_c'
overlay('abcdef' PLACING 'XY' FROM 2 FOR 2)  -- 'aXYdef'
reverse('abc')                   -- 'cba'
repeat('ab', 3)                  -- 'ababab'

split_part('a,b,c', ',', 2)      -- 'b'
string_to_array('a,b,c', ',')    -- {a,b,c}
string_to_table('a,b,c', ',')    -- 행으로 (PG14+)
array_to_string(ARRAY['a','b'], ',')  -- 'a,b'

concat('a', NULL, 'b')           -- 'ab'   (NULL 무시)
concat_ws('-', 'a', NULL, 'b')   -- 'a-b'  (구분자, NULL 무시)
'a' || NULL                      -- NULL   (연산자는 NULL 전파)

format('Hello %s, %I, %L', 'kim', 'col name', 'val')
-- %s 문자열, %I 식별자(자동 따옴표), %L 리터럴(자동 이스케이프)
-- 동적 SQL 작성 시 %I / %L 사용이 SQL 인젝션 방어의 정석

md5('abc')
encode(sha256('abc'::bytea), 'hex')
encode('abc'::bytea, 'base64') / decode('YWJj', 'base64')

to_char(1234.5, 'FM999,999.00')  -- '1,234.50'
to_number('1,234', '999,999')
ascii('A') / chr(65)
```

### 정규식
```sql
regexp_replace('a1b2', '\d', 'X', 'g')     -- 'aXbX'  (g=전역)
regexp_match('abc123', '(\d+)')            -- {123}   (첫 매치, text[])
regexp_matches('a1b2', '\d', 'g')          -- 행 여러 개 반환
regexp_split_to_array('a1b2c', '\d')       -- {a,b,c}
regexp_split_to_table('a1b2c', '\d')       -- 행으로
regexp_count('a1b2', '\d')                 -- 2 (PG15+)
regexp_substr('abc123', '\d+')             -- '123' (PG15+)
regexp_like('abc', '^a')                   -- true (PG15+)

-- 플래그: g(전역) i(대소문자무시) n(개행) x(공백무시)
```

## 2. 숫자 함수

```sql
abs(-5)              -- 5
ceil(1.1) / ceiling  -- 2
floor(1.9)           -- 1
round(1.5)           -- 2
round(1.2345, 2)     -- 1.23  (numeric 만 자릿수 지정 가능)
round(1.2345::float8, 2)  -- 에러! → round(x::numeric, 2)
trunc(1.99, 1)       -- 1.9  (버림)
mod(7, 3) / 7 % 3    -- 1
div(7, 3)            -- 2 (정수 몫)
power(2, 10) / 2^10  -- 1024
sqrt(16) / |/16      -- 4
exp / ln / log(x) / log(b, x)
sign(-3)             -- -1
random()             -- 0 <= x < 1
gen_random_uuid()    -- UUID v4 (PG13+ 내장)

-- 정수 나눗셈 함정
SELECT 1/2;              -- 0
SELECT 1::numeric/2;     -- 0.5
SELECT count(*) FILTER (WHERE x)::numeric / count(*) FROM t;  -- 비율 계산
```

## 3. 날짜/시간 함수

```sql
now() / current_timestamp        -- timestamptz, 트랜잭션 시작 시각
current_date / current_time
clock_timestamp()                -- 실제 호출 시각
localtimestamp                   -- timestamp (타임존 없음)

age(timestamp)                   -- 현재와의 차이 (interval)
age('2026-08-03', '2020-01-01')  -- 6 years 7 mons 2 days

-- extract / date_part
extract(year FROM now())         -- 2026 (numeric)
extract(epoch FROM now())        -- 유닉스 타임스탬프
extract(dow FROM now())          -- 0=일요일 ~ 6=토요일
extract(isodow FROM now())       -- 1=월요일 ~ 7=일요일
extract(quarter FROM now())
extract(epoch FROM (t2 - t1))    -- 두 시각 차이를 초로

date_trunc('month', now())       -- 2026-08-01 00:00:00+09
date_trunc('day', now())
date_trunc('hour', now())
date_trunc('week', now())        -- 월요일 시작
date_trunc('day', now(), 'UTC')  -- 타임존 지정 (PG16+)

date_bin('15 minutes', now(), '2026-01-01')  -- 임의 간격 버킷 (PG14+)

to_char(now(), 'YYYY-MM-DD HH24:MI:SS')
to_char(now(), 'YYYY"년" MM"월" DD"일"')
to_date('2026-08-03', 'YYYY-MM-DD')
to_timestamp('2026-08-03 10:00', 'YYYY-MM-DD HH24:MI')
to_timestamp(1754179200)         -- 에폭 → timestamptz

make_date(2026, 8, 3)
make_timestamptz(2026, 8, 3, 10, 0, 0, 'Asia/Seoul')
make_interval(days => 3, hours => 2)

-- 타임존 변환
now() AT TIME ZONE 'UTC'         -- timestamptz → timestamp (UTC 벽시계)
ts AT TIME ZONE 'Asia/Seoul'     -- timestamp → timestamptz
SELECT * FROM pg_timezone_names;

-- 날짜 계산
now() + interval '1 month'
now() - interval '7 days'
date_trunc('month', now()) + interval '1 month - 1 day'   -- 이번 달 마지막 날
(date_trunc('month', now()) + interval '1 month')::date - 1
```

### 날짜 포맷 패턴
| 패턴 | 의미 |
|---|---|
| `YYYY MM DD` | 연 월 일 |
| `HH24 MI SS` | 24시 분 초 (`HH`는 12시간제) |
| `MS US` | 밀리초 마이크로초 |
| `Day Dy D` | 요일명, 약어, 숫자 |
| `Month Mon` | 월 이름 |
| `TZ OF` | 타임존 약어 / 오프셋 |
| `FM` | 앞의 공백/0 제거 |

## 4. 조건 표현식

```sql
-- CASE (검색형)
CASE WHEN score >= 90 THEN 'A'
     WHEN score >= 80 THEN 'B'
     ELSE 'F'
END

-- CASE (단순형)
CASE status WHEN 'a' THEN '활성' WHEN 'i' THEN '비활성' ELSE '기타' END

COALESCE(a, b, c)          -- 첫 non-NULL
NULLIF(a, b)               -- a=b면 NULL, 아니면 a
GREATEST(a, b, c)          -- 최대값 (NULL 무시)
LEAST(a, b, c)             -- 최소값 (NULL 무시)
```

**CASE는 단축 평가되지 않을 수 있다.** 특히 집계 함수 인자에서는 모든 분기가 평가될 수 있으므로,
0으로 나누기 방지는 `NULLIF`를 쓴다.
```sql
SELECT a / NULLIF(b, 0) FROM t;   -- b=0이면 NULL
```

## 5. 집계 함수

```sql
count(*)                 -- 전체 행
count(col)               -- NULL 제외
count(DISTINCT col)
sum / avg / min / max
stddev / variance / stddev_pop / var_pop

bool_and(x) / bool_or(x)          -- 모두 참 / 하나라도 참
every(x)                          -- bool_and 별칭

string_agg(name, ', ')                          -- 문자열 연결
string_agg(name, ',' ORDER BY name)             -- 정렬 지정
array_agg(id)                                   -- 배열로
array_agg(DISTINCT id ORDER BY id)
json_agg(row_to_json(t)) / jsonb_agg(t)
jsonb_object_agg(key, value)
range_agg(period)                               -- PG14+

-- FILTER 절 (PostgreSQL/표준. CASE보다 명확)
SELECT count(*) FILTER (WHERE status = 'active')  AS active_cnt,
       count(*) FILTER (WHERE status = 'deleted') AS deleted_cnt,
       sum(amount) FILTER (WHERE type = 'in')     AS income
FROM t;

-- 순서 의존 집계 (WITHIN GROUP)
percentile_cont(0.5) WITHIN GROUP (ORDER BY score)   -- 중앙값 (보간)
percentile_disc(0.5) WITHIN GROUP (ORDER BY score)   -- 중앙값 (실제 값)
percentile_cont(ARRAY[0.25,0.5,0.75]) WITHIN GROUP (ORDER BY x)
mode() WITHIN GROUP (ORDER BY x)                     -- 최빈값
```

## 6. 집합 반환 함수 (SRF)

```sql
generate_series(1, 10)
generate_series(1, 10, 2)                       -- 1,3,5,7,9
generate_series('2026-01-01'::date, '2026-01-31', '1 day')
generate_subscripts(arr, 1)

unnest(ARRAY[1,2,3])                            -- 배열 → 행
unnest(a1, a2)                                  -- 다중 배열 병렬 전개
unnest(arr) WITH ORDINALITY AS t(val, idx)      -- 인덱스 부여

-- 존재하지 않는 날짜 채우기 (달력 조인)
SELECT d::date, coalesce(cnt, 0)
FROM generate_series('2026-08-01'::date, '2026-08-31', '1 day') d
LEFT JOIN (SELECT created_at::date dt, count(*) cnt FROM logs GROUP BY 1) s
       ON s.dt = d::date;

-- ROWS FROM: 여러 SRF 병렬 (짧은 쪽은 NULL 패딩)
SELECT * FROM ROWS FROM (generate_series(1,3), generate_series(1,5));
```

## 7. 시스템 정보 함수

```sql
version()
current_database() / current_schema() / current_user / session_user
inet_client_addr() / inet_server_addr()
pg_backend_pid()
txid_current() / pg_current_xact_id()   -- PG13+

pg_size_pretty(pg_total_relation_size('t'))   -- 인덱스 포함 크기
pg_size_pretty(pg_relation_size('t'))         -- 테이블만
pg_size_pretty(pg_database_size(current_database()))

current_setting('timezone')
current_setting('app.user_id', true)   -- true = 없으면 NULL (에러 대신)
set_config('app.user_id', '123', false)  -- false = 세션 전체, true = 트랜잭션

pg_typeof(1.5)          -- numeric
to_regclass('t')        -- 존재하면 oid, 없으면 NULL (존재 확인용)
```

## 8. 타입 변환/검사 (PG16+)

```sql
SELECT 'abc'::int;              -- 에러
SELECT CAST('abc' AS int);      -- 에러
-- PG16+ 안전 캐스팅
SELECT pg_input_is_valid('abc', 'integer');   -- false
SELECT pg_input_error_info('abc', 'integer'); -- 에러 상세
```

## 관련 문서

- [[PostgreSQL/06-WINDOW|06. 윈도우 함수]] — 집계 함수의 윈도우 형태 사용
- [[PostgreSQL/07-PLPGSQL|07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)]] — 사용자 정의 함수 작성
- [[PostgreSQL/09-POSTGRES-ONLY|09. PostgreSQL 고유 기능]] — 배열·JSONB 전용 함수
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
