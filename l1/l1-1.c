#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int main() {
    DIR *current_dir = opendir(".");
    if (current_dir == NULL)
        ERR("opendir");
    struct dirent *entity;
    struct stat entity_stats;
    int dirs = 0, files = 0, links = 0, other = 0;
    while (1) {
        errno = 0;
        entity = readdir(current_dir);
        if (entity == NULL) {
            if (errno)
                ERR("readdir"); // blad w readdir
            break; // koniec katalogu
        }

        if (lstat(entity->d_name, &entity_stats) != 0)
            ERR("lstat");

        if (S_ISDIR(entity_stats.st_mode))
            dirs++;
        else if (S_ISREG(entity_stats.st_mode))
            files++;
        else if (S_ISLNK(entity_stats.st_mode))
            links++;
        else 
            other++;
    }
    printf("Directories: %d, Files: %d, Links: %d, Other: %d\n", dirs, files, links, other);
    closedir(current_dir);
    return EXIT_SUCCESS;
}