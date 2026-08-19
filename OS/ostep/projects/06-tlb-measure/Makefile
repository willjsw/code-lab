# Makefile — OSTEP ch19 TLB 측정 실습
# 레시피 줄은 반드시 탭 문자로 시작
#
# -O1 을 쓴다: -O0 은 루프 오버헤드가 측정을 지배하고,
#   -O2 이상은 접근 루프를 통째로 최적화해 버릴 위험이 있다.
#   배열을 volatile 로 선언해 제거를 막되 최적화 수준은 낮게 유지

CC     = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -O1

all: tlb

tlb: tlb.c
	$(CC) $(CFLAGS) -o tlb tlb.c

clean:
	rm -f tlb
	rm -rf *.dSYM

.PHONY: all clean
