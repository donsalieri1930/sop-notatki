override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined,leak

ifdef CI
override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Werror -Wno-unused-parameter -Wno-unused-const-variable
endif

NAME=sop-backup

# >>> TU WPISUJESZ PLIKI ŹRÓDŁOWE JAWNIE <<<
SOURCES = track1.c strvec.c

OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o $(NAME)

clean:
	rm -f $(NAME) $(OBJECTS)
