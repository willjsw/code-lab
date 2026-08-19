# Makefile — OSTEP ch22 페이지 교체 정책 시뮬레이터
# 레시피 줄은 반드시 탭 문자로 시작

CC     = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -O2 -g

all: replace

replace: replace.c
	$(CC) $(CFLAGS) -o replace replace.c

clean:
	rm -f replace
	rm -rf *.dSYM

.PHONY: all clean
