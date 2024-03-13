SAN ?= -fsanitize=address -fsanitize=undefined
OPT ?= -O0
CCFLAGS ?= -fcolor-diagnostics -std=c++20 -Wall -Wextra -g
COMPILER ?= clang++

CC=you_have_a_non_existing_dependency
CXX=you_have_a_non_existing_dependency

%: %.cc
	$(COMPILER) $(CCFLAGS) $(SAN) $(OPT) $< -o $@

run: moves
	./moves

test: moves
	./moves > test/t1.out
	git --no-pager diff --exit-code test

clean:
	rm -f moves

.PHONY: run test clean
