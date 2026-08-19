# Makefile — OSTEP ch2 실습 4개 프로그램 빌드
#
# 레시피 줄은 반드시 탭 문자로 시작. 공백 8개는 에러("missing separator")
# make 는 타깃의 의존 파일이 타깃보다 새로우면 레시피 재실행

CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGETS = cpu mem threads io

# 첫 타깃이 기본 타깃 → 인자 없는 `make` 는 all 실행
all: $(TARGETS)

# 헤더를 의존 목록에 포함해야 헤더 수정 시 재컴파일됨
cpu: cpu.c common.h
	$(CC) $(CFLAGS) -o cpu cpu.c

mem: mem.c common.h
	$(CC) $(CFLAGS) -o mem mem.c

# -pthread 는 threads 에만 필요 (컴파일 매크로 정의 + 스레드 라이브러리 링크)
threads: threads.c common.h common_threads.h
	$(CC) $(CFLAGS) -pthread -o threads threads.c

io: io.c common.h
	$(CC) $(CFLAGS) -o io io.c

clean:
	rm -f $(TARGETS)
	rm -rf *.dSYM

# 실제 파일이 아닌 타깃 선언 → 같은 이름 파일이 있어도 항상 실행
.PHONY: all clean
