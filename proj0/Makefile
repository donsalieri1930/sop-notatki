all: p1 p2 p3 p4

p1: cpdir.c
	gcc -Wall -fsanitize=address,undefined -o p1 cpdir.c

p2: cpdir_bad.c
	gcc -Wall -fsanitize=address,undefined -o p2 cpdir_bad.c

p3: cpdir_all.c
	gcc -Wall -fsanitize=address,undefined -o p3 cpdir_all.c

p4: track1.c
	gcc -std=c17 -Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined,leak -o p4 track1.c strvec.c