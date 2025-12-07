all: p1 p2 p3 p4

p1: cpdir.c
	gcc -Wall -fsanitize=address,undefined -o p1 cpdir.c

p2: cpdir_bad.c
	gcc -Wall -fsanitize=address,undefined -o p2 cpdir_bad.c

p3: cpdir_all.c
	gcc -Wall -fsanitize=address,undefined -o p3 cpdir_all.c

p4: track1.c
	gcc -Wall -fsanitize=address,undefined -o p4 track1.c strvec.c
