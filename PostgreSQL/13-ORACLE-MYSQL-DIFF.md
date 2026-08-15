---
type: note
topic: postgresql
tags:
  - lang/sql
  - db/postgresql
  - oracle
  - mysql
  - migration
  - compatibility
  - status/verified
aliases:
  - Oracle 차이
  - MySQL 차이
created: 2026-08-15
updated: 2026-08-15
---

> **인덱스** [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]]  ·  **이전** [[PostgreSQL/12-CONVENTIONS|12. 코딩 컨벤션과 안티패턴]]  ·  **다음** [[PostgreSQL/14-TUNING|14. DB 튜닝 방법론]]

# 13. Oracle / MySQL 대비 차이점

## 1. 식별자 대소문자 — 가장 먼저 겪는 함정

| | 따옴표 없는 식별자 |
|---|---|
| Oracle | **대문자**로 변환 |
| PostgreSQL | **소문자**로 변환 |
| MySQL | OS 파일시스템에 따라 다름 (Linux는 대소문자 구분) |

```sql
-- Oracle: SELECT * FROM "USERS" 가 동작
-- PostgreSQL: SELECT * FROM "users" 가 동작
-- 해결: 큰따옴표를 쓰지 말고 전부 소문자 snake_case
```

## 2. 문법 대응표

| 기능 | Oracle | MySQL | PostgreSQL |
|---|---|---|---|
| 더미 테이블 | `FROM dual` | 생략 가능 | **생략** (`SELECT 1;`) |
| 문자열 연결 | `\|\|` | `CONCAT()` | `\|\|` 또는 `concat()` |
| NULL 대체 | `NVL(a,b)` | `IFNULL(a,b)` | `COALESCE(a,b)` |
| 조건 NULL | `NVL2(a,b,c)` | - | `CASE WHEN a IS NOT NULL...` |
| 현재 시각 | `SYSDATE` | `NOW()` | `now()` / `current_timestamp` |
| 행 수 제한 | `ROWNUM<=10` / `FETCH FIRST` | `LIMIT 10` | `LIMIT 10` / `FETCH FIRST` |
| 시퀀스 | `seq.NEXTVAL` | `AUTO_INCREMENT` | `nextval('seq')` / `IDENTITY` |
| 자동증가 | `IDENTITY` (12c+) | `AUTO_INCREMENT` | `GENERATED ALWAYS AS IDENTITY` |
| 문자열 자르기 | `SUBSTR` | `SUBSTRING` | `substring` / `substr` |
| 형변환 | `TO_CHAR/TO_NUMBER` | `CAST` | `to_char` / `::` / `CAST` |
| 빈 문자열 | `''` = NULL | `''` ≠ NULL | **`''` ≠ NULL** |
| 대소문자 무시 검색 | `UPPER(x)=...` | 기본 CI 콜레이션 | `ILIKE` |
| 정규식 | `REGEXP_LIKE` | `REGEXP` | `~` `~*` |
| UPSERT | `MERGE` | `ON DUPLICATE KEY UPDATE` | `ON CONFLICT` / `MERGE`(PG15+) |
| 계층 쿼리 | `CONNECT BY` | `WITH RECURSIVE` | **`WITH RECURSIVE`** |
| 피벗 | `PIVOT` | - | `crosstab` / `FILTER` |
| 문자열 집계 | `LISTAGG` | `GROUP_CONCAT` | `string_agg` |
| 그룹 상위 N | `ROW_NUMBER` | `ROW_NUMBER` | `DISTINCT ON` / `LATERAL` |
| 조건부 집계 | `SUM(CASE...)` | `SUM(IF...)` | `count(*) FILTER (WHERE...)` |

### 빈 문자열과 NULL — Oracle 이관 시 최대 함정
```sql
-- Oracle: '' 은 NULL 로 취급됨
-- PostgreSQL: '' 은 길이 0인 문자열. NULL 과 다름
SELECT '' IS NULL;            -- PostgreSQL: false / Oracle: true
SELECT length('');            -- PostgreSQL: 0 / Oracle: NULL

-- 이관 시 NULLIF 로 변환 필요
SELECT NULLIF(col, '') FROM t;
```

## 3. 계층 쿼리 변환 (CONNECT BY → RECURSIVE)

```sql
-- Oracle
SELECT LEVEL, id, name
FROM dept
START WITH parent_id IS NULL
CONNECT BY PRIOR id = parent_id
ORDER SIBLINGS BY name;

-- PostgreSQL
WITH RECURSIVE tree AS (
    SELECT id, parent_id, name, 1 AS lvl, ARRAY[name] AS path
    FROM dept WHERE parent_id IS NULL
    UNION ALL
    SELECT d.id, d.parent_id, d.name, t.lvl+1, t.path || d.name
    FROM dept d JOIN tree t ON d.parent_id = t.id
)
SELECT lvl, id, name FROM tree ORDER BY path;
```

| Oracle | PostgreSQL |
|---|---|
| `LEVEL` | 재귀 항에서 `lvl+1` 직접 계산 |
| `SYS_CONNECT_BY_PATH(c,'/')` | `path \|\| '/' \|\| c` 배열/문자열 누적 |
| `CONNECT_BY_ROOT c` | 앵커 항의 값을 계속 전달 |
| `CONNECT_BY_ISLEAF` | `NOT EXISTS(SELECT 1 FROM d WHERE parent_id=t.id)` |
| `ORDER SIBLINGS BY` | 누적 path 배열로 `ORDER BY` |
| `NOCYCLE` | `CYCLE` 절 (PG14+) 또는 path 배열 체크 |

## 4. 함수/프로시저 차이

| | Oracle | PostgreSQL |
|---|---|---|
| 언어 | PL/SQL | PL/pgSQL (문법 유사하나 동일하지 않음) |
| 패키지 | `CREATE PACKAGE` | **없음** → 스키마로 그룹화 |
| 함수 호출 | `SELECT f() FROM dual` | `SELECT f()` |
| 프로시저 호출 | `EXEC p()` / `CALL p()` | `CALL p()` |
| 함수 내 COMMIT | `PRAGMA AUTONOMOUS_TRANSACTION` | **불가** (dblink/pg_background 우회) |
| 프로시저 COMMIT | 가능 | 가능 (PG11+) |
| 커서 반환 | `SYS_REFCURSOR` | `refcursor` (또는 `RETURNS TABLE` 권장) |
| 예외 | `EXCEPTION WHEN ... THEN` | 동일 (예외 이름은 다름) |
| `NO_DATA_FOUND` | 자동 발생 | `SELECT INTO STRICT` 일 때만 |
| 컬렉션/BULK COLLECT | 지원 | 배열 + `array_agg` 로 대체 |
| 오버로딩 | 패키지 내 | 전역 시그니처 기준 |

```sql
-- Oracle 패키지 → PostgreSQL 스키마
-- pkg_order.calc_fee(...)  →  app_order.fn_calc_fee(...)
CREATE SCHEMA app_order;
CREATE FUNCTION app_order.fn_calc_fee(...) ...;
```

### 자율 트랜잭션 대체
PostgreSQL 함수 안에서는 COMMIT이 불가하다. 로그를 별도 커밋해야 한다면:
1. 프로시저로 만들고 `CALL` (COMMIT 가능)
2. `dblink`로 자기 자신에 접속해 별도 세션에서 실행
3. 애플리케이션 레이어에서 분리

## 5. 데이터 타입 대응

| Oracle | PostgreSQL |
|---|---|
| `VARCHAR2(n)` | `varchar(n)` 또는 `text` |
| `CLOB` | `text` |
| `BLOB` | `bytea` (또는 Large Object) |
| `NUMBER` | `numeric` |
| `NUMBER(10)` | `integer` / `bigint` |
| `NUMBER(15,2)` | `numeric(15,2)` |
| `DATE` (시분초 포함) | `timestamp` / `timestamptz` |
| `TIMESTAMP WITH TIME ZONE` | `timestamptz` |
| `RAW` | `bytea` |
| `ROWID` | `ctid` (물리 위치, 영구 식별자 아님) |
| `XMLTYPE` | `xml` |

| MySQL | PostgreSQL |
|---|---|
| `TINYINT(1)` | `boolean` |
| `DATETIME` | `timestamp` |
| `TIMESTAMP` | `timestamptz` |
| `TEXT`/`LONGTEXT` | `text` |
| `JSON` | `jsonb` |
| `ENUM('a','b')` | `CREATE TYPE ... AS ENUM` 또는 CHECK 제약 |
| `AUTO_INCREMENT` | `GENERATED ALWAYS AS IDENTITY` |
| `UNSIGNED` | **없음** → CHECK 제약으로 |
| `DOUBLE` | `double precision` |

**Oracle `DATE`는 시분초를 포함한다.** `date`로 매핑하면 시간이 잘린다 → `timestamp`로.

## 6. UPDATE 조인 문법

```sql
-- Oracle
UPDATE orders o
SET o.user_name = (SELECT u.name FROM users u WHERE u.id = o.user_id)
WHERE EXISTS (SELECT 1 FROM users u WHERE u.id = o.user_id);

-- MySQL
UPDATE orders o JOIN users u ON o.user_id = u.id SET o.user_name = u.name;

-- PostgreSQL (FROM 절 사용)
UPDATE orders o SET user_name = u.name
FROM users u WHERE o.user_id = u.id;

-- DELETE 조인
-- MySQL: DELETE o FROM orders o JOIN users u ...
-- PostgreSQL:
DELETE FROM orders o USING users u WHERE o.user_id = u.id AND u.status='deleted';
```

## 7. 트랜잭션 동작 차이

| | Oracle | MySQL | PostgreSQL |
|---|---|---|---|
| DDL 롤백 | 불가 (암묵 커밋) | 불가 (암묵 커밋) | **가능** |
| TRUNCATE 롤백 | 불가 | 불가 | **가능** |
| 에러 후 계속 진행 | 가능 | 가능 | **불가** (전체 abort) |
| 기본 격리 수준 | READ COMMITTED | REPEATABLE READ | READ COMMITTED |
| 자동 커밋 기본 | 클라이언트 설정 | ON | ON |

```sql
-- PostgreSQL: 트랜잭션 내 에러 발생 시
BEGIN;
INSERT INTO t VALUES (1);
SELECT 1/0;                -- 에러
SELECT 1;                  -- ERROR: current transaction is aborted
ROLLBACK;

-- 해결: SAVEPOINT
```

## 8. 힌트

- **PostgreSQL에는 옵티마이저 힌트가 없다.** (`/*+ INDEX(...) */` 무시됨)
- 대안: 통계 갱신(`ANALYZE`), 인덱스 설계, `enable_*` 세션 파라미터(진단용),
  `pg_hint_plan` 확장

```sql
-- 진단용 (운영 코드에 넣지 말 것)
SET enable_seqscan = off;
EXPLAIN ANALYZE SELECT ...;
RESET enable_seqscan;
```

## 9. 기타 자주 걸리는 차이

```sql
-- 1. 정수 나눗셈
-- Oracle: 1/2 = 0.5  (NUMBER 연산)
-- PostgreSQL: 1/2 = 0  (정수 연산)
SELECT 1::numeric/2;   -- 0.5

-- 2. 문자열 비교 시 공백
-- Oracle CHAR: 공백 무시 / PostgreSQL text: 공백 구분
SELECT 'a' = 'a ';     -- false

-- 3. GROUP BY 컬럼 규칙
-- MySQL(구버전): SELECT 목록에 없는 컬럼 허용 (ONLY_FULL_GROUP_BY off)
-- PostgreSQL: 엄격. PK로 그룹핑하면 종속 컬럼은 허용
SELECT id, name, count(*) FROM t GROUP BY id;   -- id가 PK면 OK

-- 4. ORDER BY 위치 번호
SELECT a, b FROM t ORDER BY 1, 2;   -- 모두 지원하나 컬럼명 명시 권장

-- 5. 대소문자 무시 정렬/비교
-- MySQL: 기본 콜레이션이 CI (대소문자 무시)
-- PostgreSQL: 기본 CS (구분). CI가 필요하면
CREATE COLLATION ci (provider = icu, locale = 'und-u-ks-level2', deterministic = false);
CREATE TABLE t (name text COLLATE ci);

-- 6. LIMIT + 업데이트
-- MySQL: UPDATE ... LIMIT 10 가능
-- PostgreSQL: 불가 → 서브쿼리 사용
UPDATE t SET a=1 WHERE id IN (SELECT id FROM t WHERE cond LIMIT 10);

-- 7. 백틱
-- MySQL: `column`
-- PostgreSQL: "column" (그리고 되도록 쓰지 말 것)

-- 8. 주석 문법
-- MySQL: # 주석 지원
-- PostgreSQL: -- 와 /* */ 만
```

## 10. 이관 체크리스트

- [ ] 큰따옴표 식별자 → 소문자 snake_case 통일
- [ ] `''`와 NULL 구분 (Oracle 이관 시 `NULLIF(x,'')` 검토)
- [ ] `NVL` → `COALESCE`, `SYSDATE` → `now()`, `dual` 제거
- [ ] Oracle `DATE` → `timestamp`/`timestamptz` (시분초 유실 주의)
- [ ] `NUMBER` → `numeric`/`integer`/`bigint` 적절히 분리
- [ ] `CONNECT BY` → `WITH RECURSIVE`
- [ ] `MERGE` → `ON CONFLICT` (PG15 미만) 또는 `MERGE`
- [ ] 패키지 → 스키마 + 함수
- [ ] 함수 내 COMMIT → 프로시저로 전환
- [ ] `SYS_REFCURSOR` → `RETURNS TABLE`
- [ ] `ROWNUM` → `LIMIT` / `row_number()`
- [ ] 옵티마이저 힌트 제거 후 인덱스/통계로 재튜닝
- [ ] 정수 나눗셈 검토 (`1/2 = 0`)
- [ ] 시퀀스 → IDENTITY, 시작값 이관
- [ ] 에러 후 계속 진행하는 로직 → SAVEPOINT 추가
- [ ] 트리거/제약 이름 충돌 확인 (PostgreSQL은 스키마 단위 유니크)

## 관련 문서

- [[PostgreSQL/01-BASICS|01. 기본 문법과 데이터 타입]] — 식별자·타입 기본 규칙
- [[PostgreSQL/07-PLPGSQL|07. FUNCTION / PROCEDURE 작성 (PL/pgSQL)]] — PL/SQL 대비 PL/pgSQL 작성법
- [[PostgreSQL/09-POSTGRES-ONLY|09. PostgreSQL 고유 기능]] — PostgreSQL에만 있는 기능
- [[PostgreSQL/00-INDEX|PostgreSQL 문법 총정리]] — 전체 문서 인덱스
