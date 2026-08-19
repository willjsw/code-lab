# Makefile — OSTEP ch17 프리 공간 관리 실습
# 레시피 줄은 반드시 탭 문자로 시작

CC     = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g

all: myalloc

myalloc: myalloc.c
	$(CC) $(CFLAGS) -o myalloc myalloc.c

clean:
	rm -f myalloc
	rm -rf *.dSYM

.PHONY: all clean
