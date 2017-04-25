all: echo_s echo_c log_s

echo_s: echo_s.c utilities.c
	gcc -std=c99 echo_s.c utilities.c -o echo_s

echo_c: echo_c.c utilities.c
	gcc -std=c99 echo_c.c utilities.c -o echo_c

log_s: log_s.c utilities.c
	gcc -std=c99 log_s.c utilities.c -o log_s

clean:
	rm -f echo_s echo_c log_s
