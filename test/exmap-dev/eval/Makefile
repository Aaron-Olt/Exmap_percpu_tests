TESTS = \
	test-bench-alloc \
	test-bench-read  \
	test-bench-steal \
	test-exmap

alloc: 
	gcc -g -O -c perf.c
	g++ -o test-bench-alloc  test-bench-alloc.cc perf.o -O3 -lpthread -ggdb

all: ${TESTS}

clean:
	rm -f ${TESTS}

test-%: test-%.cc
	g++ -o $@  $< -O3 -lpthread -ggdb


clean_alloc:
	rm -f perf.o
	rm -f test-bench-alloc
