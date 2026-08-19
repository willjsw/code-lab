# Makefile — OSTEP ch14 메모리 오류 재현 · 검출 실습
# 레시피 줄은 반드시 탭 문자로 시작
#
# 두 종류로 빌드한다
#   일반 빌드  : 실제 프로그램이 어떻게 죽는지(또는 조용히 잘못 동작하는지) 관찰
#   ASan 빌드  : AddressSanitizer 로 오류 지점을 정확히 지목
#
# macOS(특히 Apple Silicon)에서는 valgrind 가 사실상 사용 불가 →
#   원서의 valgrind 역할을 AddressSanitizer(-fsanitize=address)와
#   macOS 기본 제공 leaks 명령이 대체한다

CC        = gcc
CFLAGS    = -Wall -Wextra -g
ASANFLAGS = -Wall -Wextra -g -fsanitize=address -fno-omit-frame-pointer

SRCS    = $(wildcard 0*.c)
PLAIN   = $(SRCS:.c=)
ASAN    = $(SRCS:.c=-asan)

all: $(PLAIN) $(ASAN)

# 일반 빌드
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# ASan 빌드
%-asan: %.c
	$(CC) $(ASANFLAGS) -o $@ $<

clean:
	rm -f $(PLAIN) $(ASAN)
	rm -rf *.dSYM

.PHONY: all clean
