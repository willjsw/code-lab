---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - array
  - jsonb
  - range-type
  - extension
  - copy
  - listen-notify
  - status/verified
aliases:
  - JSONB
  - 배열 타입
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/08-CALLING|08. 프로시저 / 함수 호출 방법]]  ·  **다음** [[PostgreSQL/10-TRANSACTION|10. 트랜잭션, 격리 수준, 락]]

# 09. PostgreSQL 고유 기능

다른 DBMS에는 없거나 문법이 다른, PostgreSQL만의 기능 모음.

## 1. 배열 (Array)

```sql
CREATE TABLE t (tags text[], scores int[], matrix int[][]);

-- 리터럴
'{1,2,3}'::int[]
ARRAY[1,2,3]
ARRAY[['a','b'],['c','d']]      -- 2차원
'{}'::int[]                      -- 빈 배열 (NULL과 다름)

-- 인덱스는 1부터 시작
SELECT (ARRAY[10,20,30])[1];        -- 10
SELECT (ARRAY[10,20,30])[2:3];      -- {20,30}  슬라이스
SELECT (ARRAY[10,20,30])[5];        -- NULL (범위 밖은 에러 아님)
```

### 배열 연산자
```sql
ARRAY[1,2] || ARRAY[3]          -- {1,2,3}  연결
1 = ANY(ARRAY[1,2,3])           -- true     포함 여부 (IN 대용)
1 <> ALL(ARRAY[1,2,3])          -- false
ARRAY[1,2] @> ARRAY[1]          -- true     포함(contains)
ARRAY[1] <@ ARRAY[1,2]          -- true     포함됨
ARRAY[1,2] && ARRAY[2,3]        -- true     교집합 존재(overlap)
ARRAY[1,2] = ARRAY[1,2]         -- true
```

### 배열 함수
```sql
array_length(arr, 1)            -- 1차원 길이
cardinality(arr)                -- 전체 원소 수
array_append(arr, 4) / arr || 4
array_prepend(0, arr)
array_remove(arr, 2)            -- 값으로 제거
array_replace(arr, 1, 9)
array_position(arr, 'x')        -- 위치 (없으면 NULL)
array_positions(arr, 'x')       -- 모든 위치 배열
array_cat(a1, a2)
array_to_string(arr, ',')
string_to_array('a,b', ',')
array_fill(0, ARRAY[3])         -- {0,0,0}
array_agg(x)                    -- 집계
unnest(arr)                     -- 행으로 전개
array_dims(arr)                 -- '[1:3]'
trim_array(arr, 1)              -- 뒤에서 n개 제거 (PG14+)
```

### 배열 활용 패턴
```sql
-- 배열 → 행
SELECT * FROM unnest(ARRAY['a','b','c']) AS t(tag);
SELECT id, tag FROM t, unnest(t.tags) AS tag;

-- 행 → 배열
SELECT user_id, array_agg(tag ORDER BY tag) FROM tags GROUP BY user_id;

-- 배열 정렬 / 중복 제거
SELECT array_agg(DISTINCT x ORDER BY x) FROM unnest(arr) x;

-- IN 대신 배열 바인딩 (파라미터 1개로 처리 — 매우 유용)
SELECT * FROM t WHERE id = ANY($1::int[]);
-- JDBC:  ps.setArray(1, conn.createArrayOf("int", ids));

-- 배열 인덱스 (GIN)
CREATE INDEX ON t USING gin (tags);
SELECT * FROM t WHERE tags @> ARRAY['sale'];
```

## 2. JSON / JSONB

**`jsonb`를 쓴다.** `json`은 원문 텍스트를 그대로 보관(공백/키순서/중복키 유지, 파싱 매번),
`jsonb`는 이진 저장(중복키 제거, 키순서 미보장, 인덱싱 가능).

```sql
CREATE TABLE t (data jsonb NOT NULL DEFAULT '{}'::jsonb);
```

### 접근 연산자
```sql
data -> 'key'            -- jsonb 반환
data ->> 'key'           -- text 반환
data -> 0                -- 배열 인덱스 (jsonb)
data ->> 0               -- 배열 인덱스 (text)
data #> '{a,b}'          -- 경로, jsonb
data #>> '{a,b}'         -- 경로, text
data #>> '{items,0,name}'
```
**규칙: 화살표 하나 `->`는 JSON, 두 개 `->>`는 텍스트.** 마지막 단계에서만 `->>`.

### 포함/존재 연산자
```sql
data @> '{"a":1}'              -- 포함 (GIN 인덱스 사용됨)
data <@ '{"a":1,"b":2}'        -- 포함됨
data ?  'key'                  -- 키 존재
data ?| ARRAY['a','b']         -- 키 중 하나라도 존재
data ?& ARRAY['a','b']         -- 키 전부 존재
```

### 수정 연산자/함수
```sql
data || '{"b":2}'::jsonb                    -- 병합 (얕은 병합)
data - 'key'                                -- 키 제거
data - ARRAY['a','b']                       -- 여러 키 제거
data #- '{a,b}'                             -- 경로 제거

jsonb_set(data, '{a,b}', '"new"'::jsonb)              -- 값 설정
jsonb_set(data, '{a}', '1', true)                     -- true=없으면 생성
jsonb_set_lax(data, '{a}', NULL, true, 'use_json_null')  -- PG13+
jsonb_insert(data, '{arr,0}', '"x"', true)            -- 배열 삽입
```

### 생성/변환 함수
```sql
to_jsonb(x)
jsonb_build_object('a', 1, 'b', 'x')
jsonb_build_array(1, 'a', true)
jsonb_object(ARRAY['a','1','b','2'])
row_to_json(t) / to_jsonb(t)
jsonb_agg(x)
jsonb_object_agg(key, val)
jsonb_pretty(data)

-- 행 → JSON
SELECT jsonb_agg(to_jsonb(t)) FROM users t;
SELECT jsonb_build_object('id', id, 'name', name, 'tags', tags) FROM users;
```

### 전개 함수
```sql
jsonb_each(data)                  -- (key, value jsonb) 행
jsonb_each_text(data)             -- (key, value text) 행
jsonb_object_keys(data)           -- 키 목록
jsonb_array_elements(arr)         -- 배열 원소 → 행 (jsonb)
jsonb_array_elements_text(arr)    -- 배열 원소 → 행 (text)
jsonb_array_length(arr)
jsonb_typeof(data->'a')           -- 'object','array','string','number','boolean','null'

-- JSON → 관계형 (매우 유용)
SELECT * FROM jsonb_to_record('{"a":1,"b":"x"}'::jsonb) AS t(a int, b text);
SELECT * FROM jsonb_to_recordset('[{"a":1},{"a":2}]'::jsonb) AS t(a int);
SELECT * FROM jsonb_populate_record(NULL::users, data);
```

```sql
-- 실전: JSON 배열 안 객체 펼치기
SELECT o.id, item->>'sku' AS sku, (item->>'qty')::int AS qty
FROM orders o,
     jsonb_array_elements(o.data->'items') AS item;
```

### JSONPath (PG12+)
```sql
data @? '$.items[*] ? (@.qty > 10)'        -- 존재 여부 (boolean)
data @@ '$.a == 1'                         -- 조건 판정 (boolean)
jsonb_path_query(data, '$.items[*].sku')   -- 결과 행들
jsonb_path_query_array(data, '$.items[*].sku')
jsonb_path_query_first(data, '$.items[0]')
jsonb_path_exists(data, '$.a')

-- 변수 바인딩
jsonb_path_query(data, '$.items[*] ? (@.qty > $min)', '{"min":10}')
```

### JSONB 인덱스
```sql
-- 기본 GIN: @>, ?, ?|, ?&, @?, @@ 모두 지원. 크기 큼
CREATE INDEX ON t USING gin (data);

-- jsonb_path_ops: @>, @?, @@ 만 지원. 더 작고 빠름
CREATE INDEX ON t USING gin (data jsonb_path_ops);

-- 특정 키만 B-tree (등호/범위 조회에 유리)
CREATE INDEX ON t ((data->>'status'));
SELECT * FROM t WHERE data->>'status' = 'active';

-- 숫자 비교는 캐스팅 인덱스
CREATE INDEX ON t (((data->>'qty')::int));
```

**주의:** `data->>'x' = 'y'` 는 GIN 인덱스를 못 쓴다. `data @> '{"x":"y"}'`로 써야
GIN이 동작한다. 아니면 표현식 B-tree 인덱스를 따로 만든다.

## 3. 범위 타입 (Range)

```sql
int4range, int8range, numrange, tsrange, tstzrange, daterange
-- PG14+: 다중 범위 int4multirange 등

'[1,10)'::int4range        -- 1 이상 10 미만 (기본: 하한 포함, 상한 제외)
'[2026-01-01,2026-12-31]'::daterange
tstzrange(now(), now() + interval '1 day', '[)')
numrange(1, NULL)          -- 무한대

-- 연산자
r @> 5                     -- 값 포함
r1 @> r2                   -- 범위 포함
r1 && r2                   -- 겹침 (overlap)
r1 -|- r2                  -- 인접
r1 + r2 / r1 * r2 / r1 - r2  -- 합집합/교집합/차집합
lower(r) / upper(r) / isempty(r)

-- 겹침 방지 제약 (예약 시스템의 정석)
CREATE TABLE booking (
    room_id int,
    period  tstzrange,
    EXCLUDE USING gist (room_id WITH =, period WITH &&)
);
```

## 4. 사용자 정의 타입

```sql
-- ENUM
CREATE TYPE order_status AS ENUM ('pending','paid','shipped','done');
CREATE TABLE t (status order_status NOT NULL DEFAULT 'pending');
-- 정렬은 정의 순서대로
ALTER TYPE order_status ADD VALUE 'canceled';               -- 끝에 추가
ALTER TYPE order_status ADD VALUE 'refund' AFTER 'paid';    -- 위치 지정
-- 값 삭제/변경은 불가 → 자주 바뀌면 코드 테이블 + FK 가 낫다

-- 복합 타입
CREATE TYPE addr AS (city text, street text, zip text);
CREATE TABLE t (home addr);
INSERT INTO t VALUES (ROW('서울','강남대로','06000'));
SELECT (home).city FROM t;
UPDATE t SET home.city = '부산';

-- 도메인 (제약 붙은 타입)
CREATE DOMAIN email AS text CHECK (VALUE ~ '^[^@]+@[^@]+\.[^@]+$');
CREATE DOMAIN positive_int AS int CHECK (VALUE > 0) NOT NULL;
CREATE TABLE t (mail email);
```

## 5. 확장 (Extension)

```sql
CREATE EXTENSION IF NOT EXISTS pg_trgm;
SELECT * FROM pg_available_extensions;
SELECT * FROM pg_extension;
```

| 확장 | 용도 |
|---|---|
| `pg_stat_statements` | 쿼리 통계 (성능 튜닝 필수) |
| `pg_trgm` | 유사도 검색, `LIKE '%x%'` 인덱스 |
| `pgcrypto` | 암호화, 해시 (`crypt`, `gen_salt`) |
| `uuid-ossp` | UUID 생성 (PG13+ 는 `gen_random_uuid()` 내장이라 불필요) |
| `hstore` | 키-값 저장 (jsonb가 대체) |
| `postgres_fdw` | 다른 PostgreSQL 연결 |
| `dblink` | 동적 원격 쿼리 |
| `tablefunc` | `crosstab` (피벗) |
| `btree_gin` / `btree_gist` | GIN/GiST에 일반 타입 결합 |
| `pgvector` | 벡터 유사도 검색 (서드파티) |

```sql
-- pg_trgm: 부분 문자열 검색 가속
CREATE EXTENSION pg_trgm;
CREATE INDEX ON t USING gin (name gin_trgm_ops);
SELECT * FROM t WHERE name LIKE '%검색어%';    -- 인덱스 사용됨
SELECT name, similarity(name, '검색어') FROM t ORDER BY name <-> '검색어' LIMIT 10;

-- crosstab (피벗)
CREATE EXTENSION tablefunc;
SELECT * FROM crosstab(
    'SELECT dept, month, amt FROM sales ORDER BY 1,2',
    'SELECT DISTINCT month FROM sales ORDER BY 1'
) AS t(dept text, m1 numeric, m2 numeric, m3 numeric);
```

## 6. 전문 검색 (Full Text Search)

```sql
SELECT to_tsvector('english', 'The quick brown fox');
SELECT to_tsvector('simple', '검색 대상 문장');   -- 한국어는 simple 또는 형태소 확장 필요

SELECT * FROM docs WHERE to_tsvector('english', body) @@ to_tsquery('english', 'quick & fox');
SELECT * FROM docs WHERE tsv @@ plainto_tsquery('quick fox');       -- 평문 입력
SELECT * FROM docs WHERE tsv @@ phraseto_tsquery('quick brown');    -- 구문 검색
SELECT * FROM docs WHERE tsv @@ websearch_to_tsquery('"quick fox" -slow');  -- 웹 검색 문법

-- 생성 컬럼 + GIN 인덱스 (권장)
ALTER TABLE docs ADD COLUMN tsv tsvector
    GENERATED ALWAYS AS (to_tsvector('english', coalesce(title,'') || ' ' || coalesce(body,''))) STORED;
CREATE INDEX ON docs USING gin (tsv);

-- 순위 / 하이라이트
SELECT ts_rank(tsv, q), ts_headline(body, q)
FROM docs, to_tsquery('fox') q WHERE tsv @@ q ORDER BY 1 DESC;
```

**한국어 전문 검색은 기본 지원이 없다.** `simple` 설정 + `pg_trgm`, 또는
`textsearch_ko`/`mecab-ko` 같은 외부 확장이 필요하다 (환경별 확인 필요).

## 7. LISTEN / NOTIFY — 비동기 메시징

```sql
LISTEN my_channel;
NOTIFY my_channel, '페이로드';
SELECT pg_notify('my_channel', 'payload');   -- 동적 채널명
UNLISTEN my_channel;
```
- 페이로드 최대 8000 바이트
- 커밋 시점에 전달. 롤백하면 전달 안 됨
- 리스너가 없으면 메시지 소실 (영속 큐 아님)

## 8. Advisory Lock — 애플리케이션 정의 락

```sql
SELECT pg_advisory_lock(12345);              -- 세션 락 (명시적 해제 필요)
SELECT pg_try_advisory_lock(12345);          -- 즉시 반환 (boolean)
SELECT pg_advisory_unlock(12345);
SELECT pg_advisory_xact_lock(12345);         -- 트랜잭션 락 (커밋 시 자동 해제, 권장)
SELECT pg_advisory_lock(1, 2);               -- 2개 int 키

-- 배치 중복 실행 방지
SELECT pg_try_advisory_lock(hashtext('daily_batch'));
```

## 9. COPY — 대량 입출력

```sql
-- 서버 파일 (superuser/pg_read_server_files 권한 필요)
COPY t FROM '/data/f.csv' WITH (FORMAT csv, HEADER true, DELIMITER ',');
COPY t TO   '/data/f.csv' WITH (FORMAT csv, HEADER true);
COPY (SELECT * FROM t WHERE x) TO '/data/f.csv' WITH (FORMAT csv);

-- 클라이언트 파일 (psql — 권한 불필요)
\copy t FROM 'local.csv' WITH (FORMAT csv, HEADER true)
\copy (SELECT * FROM t) TO 'out.csv' WITH (FORMAT csv, HEADER true)

-- 옵션
WITH (FORMAT csv, HEADER, NULL '', QUOTE '"', ESCAPE '\', ENCODING 'UTF8',
      FORCE_NULL (col), ON_ERROR ignore)   -- ON_ERROR 는 PG17+
```
**INSERT 반복보다 수십 배 빠르다.** 대량 적재는 COPY를 쓴다.

## 10. FILTER 절 / 기타 편의 문법

```sql
-- FILTER (조건부 집계, CASE보다 명확)
SELECT count(*) FILTER (WHERE status='a'), count(*) FILTER (WHERE status='b') FROM t;

-- ILIKE (대소문자 무시)
SELECT * FROM t WHERE name ILIKE '%kim%';

-- DISTINCT ON
SELECT DISTINCT ON (user_id) * FROM orders ORDER BY user_id, created_at DESC;

-- 정규식 연산자
SELECT * FROM t WHERE name ~* '^kim';

-- 다중 행 VALUES 조인
SELECT * FROM (VALUES (1,'a'),(2,'b')) v(id, nm);

-- ::캐스팅
SELECT '1'::int;

-- IS DISTINCT FROM (NULL 안전 비교)
SELECT * FROM t WHERE a IS DISTINCT FROM b;

-- 배열 첨자 슬라이스, 문자열 첨자
SELECT ('abcdef')[2];      -- 불가 (문자열은 substring 사용)
```

## 11. 시스템 카탈로그 조회

```sql
-- 테이블 목록
SELECT schemaname, tablename FROM pg_tables WHERE schemaname='app';

-- 컬럼 정보
SELECT column_name, data_type, is_nullable, column_default
FROM information_schema.columns WHERE table_name='users' ORDER BY ordinal_position;

-- 인덱스 정의
SELECT indexname, indexdef FROM pg_indexes WHERE tablename='users';

-- 제약조건
SELECT conname, pg_get_constraintdef(oid) FROM pg_constraint
WHERE conrelid = 'users'::regclass;

-- 테이블 크기 상위
SELECT relname, pg_size_pretty(pg_total_relation_size(relid)) AS size
FROM pg_stat_user_tables ORDER BY pg_total_relation_size(relid) DESC LIMIT 10;

-- 현재 실행 중인 쿼리
SELECT pid, now()-query_start AS dur, state, left(query,80)
FROM pg_stat_activity WHERE state != 'idle' ORDER BY dur DESC;

-- 락 대기 확인
SELECT blocked.pid AS blocked_pid, blocking.pid AS blocking_pid,
       left(blocked.query,60) AS blocked_query
FROM pg_stat_activity blocked
JOIN pg_stat_activity blocking ON blocking.pid = ANY(pg_blocking_pids(blocked.pid))
WHERE cardinality(pg_blocking_pids(blocked.pid)) > 0;

-- 세션 종료
SELECT pg_cancel_backend(pid);      -- 쿼리만 취소 (안전)
SELECT pg_terminate_backend(pid);   -- 연결 강제 종료
```

## 관련 문서

- [[PostgreSQL/01-BASICS|01. 기본 문법과 데이터 타입]] — 기본 데이터 타입과의 관계
- [[PostgreSQL/05-FUNCTIONS|05. 내장 함수]] — 고유 타입 전용 함수·연산자
- [[PostgreSQL/13-ORACLE-MYSQL-DIFF|13. Oracle / MySQL 대비 차이점]] — 타 DBMS에 대응 기능이 없는 지점
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
