all: p1 p2 p3 p4
p1: prog13b.c	
	gcc -Wall -fsanitize=address,undefined -o p1 prog13b.c
p2: prog14.c
	gcc -Wall -fsanitize=address,undefined -o p2 prog14.c
p3: prog15b.c
	gcc -Wall -fsanitize=address,undefined -o p3 prog15b.c
p4: prog16b.c
	gcc -Wall -fsanitize=address,undefined -o p4 prog16b.c
.PHONY: clean all
clean:
	rm p1 p2 p3 p4
