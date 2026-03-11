CC = gcc
CFLAGS = -I/usr/local/include/rbus -I/usr/local/include/rtmessage -I/work/rbus/src/rbus -I/work/rbus/src/core -I/work/rbus/src/rtmessage -g -O0 -fprofile-arcs -ftest-coverage
LDFLAGS = -L/usr/local/lib -lrtMessage -lpthread -lrbus

test_notify: test_notify.c
	$(CC) $(CFLAGS) test_notify.c -o test_notify $(LDFLAGS)

clean:
	rm -f test_notify *.gcno *.gcda *.gcov
