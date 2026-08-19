# Makefile — OSTEP ch13 주소 공간 실습
# 레시피 줄은 반드시 탭 문자로 시작
#
# -Wno-unused-parameter — 원서 코드가 main(int argc, char *argv[]) 원형을 유지하면서
#   인자를 쓰지 않으므로, 원형 보존을 위해 이 경고만 끈다

CC      = gcc
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -g
TARGETS = va layout

all: $(TARGETS)

va: va.c
	$(CC) $(CFLAGS) -o va va.c

layout: layout.c
	$(CC) $(CFLAGS) -o layout layout.c

clean:
	rm -f $(TARGETS)
	rm -rf *.dSYM

.PHONY: all clean
