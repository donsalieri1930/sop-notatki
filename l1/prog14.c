#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


void usage(char *pname)
{
    fprintf(stderr, "USAGE:%s source destination\n", pname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 3)
        usage(argv[0]);
    char *src = argv[1];
    char *dest = argv[2];
    
    int fd = open(src, O_RDONLY); // otworz source
    if (fd == -1)
        ERR("open");
    
    struct stat statbuf; // oblicz rozmiar
    if (stat(src, &statbuf) == -1)
        ERR("stat");
    int size = statbuf.st_size;

    char *srcbuf = (char*)malloc(size); // zapisz bajty do tablicy
    if (srcbuf == NULL)
        ERR("malloc");
    if (read(fd, srcbuf, size) != size)
        ERR("read");

    if (close(fd) == -1) // zamknij source
        ERR("close");

    fd = open(dest, O_WRONLY); // otworz dest
    if (fd == -1)
        ERR("open");

    if (write(fd, srcbuf, size) != size) // zapisz bajty do dest
        ERR("write");

    free(srcbuf);
    srcbuf = NULL;

    if (close(fd) == -1) // zamknij dest
        ERR("close");
    
    printf("Copied %d bytes from %s to %s\n", size, src, dest);
    return EXIT_SUCCESS;
}