CFLAGS=-Wall -Wextra -lcurses -lSDL2

all: atarascii

atarascii: main.o mos6507.o mos6507_trace.o mem.o tia.o pia.o cart.o console.o gui.o audio.o tas.o
	gcc -o atarascii $^ ${CFLAGS}

main.o: main.c
	gcc -c $^ ${CFLAGS}

mos6507.o: mos6507.c
	gcc -c $^ ${CFLAGS}

mos6507_trace.o: mos6507_trace.c
	gcc -c $^ ${CFLAGS}

mem.o: mem.c
	gcc -c $^ ${CFLAGS}

tia.o: tia.c
	gcc -c $^ ${CFLAGS}

pia.o: pia.c
	gcc -c $^ ${CFLAGS}

cart.o: cart.c
	gcc -c $^ ${CFLAGS}

console.o: console.c
	gcc -c $^ ${CFLAGS}

gui.o: gui.c
	gcc -c $^ ${CFLAGS}

audio.o: audio.c
	gcc -c $^ ${CFLAGS}

tas.o: tas.c
	gcc -c $^ ${CFLAGS}

.PHONY: clean
clean:
	rm -f *.o atarascii

