all: prog1 prog2 prog3 prog4 prog5 prog6

prog1: prog1.c
	gcc -Wall -fsanitize=address,undefined prog1.c -o prog1

prog2: prog2.c
	gcc -Wall -fsanitize=address,undefined prog2.c -o prog2

prog3: prog3.c
	gcc -Wall -fsanitize=address,undefined prog3.c -o prog3

prog4: prog4.c
	gcc -Wall -fsanitize=address,undefined prog4.c -o prog4

prog5: prog5.c
	gcc -Wall -fsanitize=address,undefined prog5.c -o prog5

prog6: prog6.c
	gcc -Wall -fsanitize=address,undefined prog6.c -o prog6

clean:
	rm prog1 prog2 prog3 prog4 prog6
