# Makefile — OSTEP ch5 프로세스 API 실습
# 레시피 줄은 반드시 탭 문자로 시작
#
# -Wno-unused-parameter — 원서 코드가 main(int argc, char *argv[]) 원형을 유지하면서
#   인자를 쓰지 않는 경우가 많음. 원형을 보존하기 위해 이 경고만 끈다

CC      = gcc
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -g
TARGETS = p1 p2 p3 p4

all: $(TARGETS)

p1: p1.c
	$(CC) $(CFLAGS) -o p1 p1.c

p2: p2.c
	$(CC) $(CFLAGS) -o p2 p2.c

p3: p3.c
	$(CC) $(CFLAGS) -o p3 p3.c

p4: p4.c
	$(CC) $(CFLAGS) -o p4 p4.c

clean:
	rm -f $(TARGETS) p4.output
	rm -rf *.dSYM

.PHONY: all clean
