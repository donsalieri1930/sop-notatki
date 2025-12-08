#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <ftw.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/inotify.h>
#include <poll.h>
#include <sys/stat.h>
#include <dirent.h>

#include "strvec.h"

#define MAXFD 20
#define COPY_BUF_SIZE 4096
#define INOTIFY_BUF_SIZE 4096
#define INOTIFY_EVENTS (IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM | IN_DELETE_SELF | IN_MOVE_SELF)

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name) {
    fprintf(stderr, "USAGE: %s src dest\n", name);
    exit(EXIT_FAILURE);
}

ssize_t bulk_read(int fd, char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, const char *buf, size_t count) {
    ssize_t c;
    ssize_t len = 0;
    do {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void try_copy(int src_fd, int dest_fd) {
    char buf[COPY_BUF_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = bulk_read(src_fd, buf, sizeof(buf))) > 0) {
        if (bulk_write(dest_fd, buf, bytes_read) == -1)
            ERR("bulk_write");
    }
    if (bytes_read == -1)
        ERR("bulk_read");
}

char *join_paths(const char *path1, const char *path2) {
    /* Join any two paths. Returned pointer must be freed. */

    int path1_len = strlen(path1);
    int path2_len = strlen(path2);
    char *path12 = malloc((path1_len + 1 + path2_len + 1)*sizeof(char));
    if (path12 == NULL)
        ERR("malloc");
    if (path1_len == 0) {
        memcpy(path12, path2, path2_len + 1);
        return path12;
    }
    if (path2_len == 0) {
        memcpy(path12, path1, path1_len + 1);
        return path12;
    }
    if (path1[path1_len - 1] == '/')
        path1_len--;
    memcpy(path12, path1, path1_len);
    path12[path1_len] = '/';
    int path2_slash = 0;
    if (path2[0] == '/') {
        path2_len--;
        path2_slash = 1;
    }
    memcpy(path12 + path1_len + 1, path2 + path2_slash, path2_len + 1);
    return path12;
}

int src_wd = -1;

void try_backup_dir(const char *src, const struct stat *sb, const char *dest, int fd, strvec *wds) {
    if (mkdir(dest, sb->st_mode) == -1)
        ERR("mkdir");
    int wd = inotify_add_watch(fd, src, INOTIFY_EVENTS);
    if (wd == -1)
        ERR("inotify_add_watch");
    if (src_wd == -1)
        src_wd = wd;
    if (strvec_set(wds, wd, src) == -1)
        ERR("strvec_set");

    /* strvec will create its own copy. */
}

void try_backup_file(const char *src, const struct stat *sb, const char *dest) {
    int src_fd = open(src, O_RDONLY);
    int dest_fd = open(dest, O_CREAT | O_TRUNC | O_WRONLY, sb->st_mode);
    if (src_fd == -1 || dest_fd == -1)
        ERR("open");
    try_copy(src_fd, dest_fd);
    if (close(src_fd) == -1 || close(dest_fd) == -1)
        ERR("close");
}

void try_backup_sl(const char *src, const char *src_root_abs, int src_root_abs_len,
    const char *dest_root_abs, const char *dest) {

    char sl[PATH_MAX + 1];
    int sl_len = readlink(src, sl, sizeof(sl) - 1);
    if (sl_len == -1) 
        ERR("readlink");
    sl[sl_len] = 0;

    /* If link target is an absolute path, and inside source directory, 
        create a symlink to a copy. Otherwise use the same path. */

    if (strncmp(src_root_abs, sl, src_root_abs_len) == 0 &&
        (sl[src_root_abs_len] == '/' || sl[src_root_abs_len] == 0)) {

        /* Comparing src_root_abs chars is not sufficient. For example:
        /abs/src
        /abs/src2/sl 
        sl appears to be inside source. */

        char *sl_suffix = sl + src_root_abs_len;
        char *new_sl = join_paths(dest_root_abs, sl_suffix);
        if (symlink(new_sl, dest) == -1)
            ERR("symlink");
        free(new_sl);
    } else {
        if (symlink(sl, dest) == -1)
                ERR("symlink");
    }
}

/* nftw state */

char src_abs[PATH_MAX + 1];
char dest_abs[PATH_MAX + 1];
int src_abs_len;
int nftw_root_len;
int at_src;
int inotify_fd;
strvec inotify_wds;

int walk_backup(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    /* Path to source directory inside fpath is not guaranteed to be
       equal to the path passed by nftw. During the initial walk of
       the source directory, we calculate the length of source fpath. 
       Any additional nftw calls will pass only fpath values, so path 
       prefix (path to source) does not change. */

    if (at_src && ftwbuf->level == 0) {
        nftw_root_len = strlen(fpath);
    }

    const char *src_suffix = fpath + nftw_root_len;
    char *dest_fpath_abs = join_paths(dest_abs, src_suffix);
    switch (tflag) {
        case FTW_D:
            try_backup_dir(fpath, sb, dest_fpath_abs, inotify_fd, &inotify_wds);
            break;

        case FTW_F:
            try_backup_file(fpath, sb, dest_fpath_abs);
            break;

        case FTW_SL:
            try_backup_sl(fpath, src_abs, src_abs_len, 
                dest_abs, dest_fpath_abs);
            break;
        default:
            break;
    }
    free(dest_fpath_abs);
    return 0;
}

int walk_remove(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if (remove(fpath) == -1)
        ERR("remove");
    return 0;
}

volatile sig_atomic_t stop_loop = 0;

void handle_events() {
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[INOTIFY_BUF_SIZE] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t size;

    while(1) {
        size = read(inotify_fd, buf, sizeof(buf));
        if (size == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */

        if (size == -1)
            break;

        /* Loop over all events in the buffer. */

        for (char *ptr = buf; ptr < buf + size; ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            /* Check if source directory was deleted or moved. In that 
               case, we stop the program. */

            if (event->wd == src_wd && (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))) { 
                stop_loop = 1;
                return;
            }

            char *dir = inotify_wds.strings[event->wd];
            if (dir == NULL) {
            
                /* Event was triggered by a directory that is no longer 
                   watched. This can be caused by this event being queued
                   after the watch was removed, or by IN_IGNORED, which is 
                   triggered by removing a watch. In both cases, we do nothing. */

                continue;
                
            }
            char *fpath = join_paths(dir, event->name);
            char *dest_fpath = join_paths(dest_abs, fpath + nftw_root_len);

            /* Event for a directory. */

            if (event->mask & IN_ISDIR) {
                if (event->mask & (IN_CREATE | IN_MOVED_TO)) {

                    /* Directory was created or moved, and may not be empty.
                       Therefore we must add all files, directories and watches
                       recursively. */
                    
                    if (nftw(fpath, walk_backup, MAXFD, FTW_PHYS) == -1)
                        ERR("nftw");
                }
                
                else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {

                    /* Directory was deleted or moved, could be not empty.
                       Paths are no longer valid, so we must remove all 
                       watches by fpath prefix. */

                    int fpath_len = strlen(fpath);
                    for (int wd=0; wd<=inotify_wds.max_i; wd++) {

                        /* strncmp will crash with NULL pointer */

                        if (inotify_wds.strings[wd] != NULL) {
                            if (strncmp(fpath, inotify_wds.strings[wd], fpath_len) == 0 &&
                                (inotify_wds.strings[wd][fpath_len] == '/' || inotify_wds.strings[wd][fpath_len] == 0)) {
                                    inotify_rm_watch(inotify_fd, wd);
                                    strvec_set(&inotify_wds, wd, NULL);
                            }
                        }
                    }
                    
                    /* Remove directory from destination. */

                    if (nftw(dest_fpath, walk_remove, MAXFD, FTW_PHYS | FTW_DEPTH) == -1)
                        ERR("nftw");
                }
            }

            /* Event for a file. */
            
            else {
                struct stat sb;

                if (event->mask & (IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE)) {

                    /* Regardles of a specific event we create or truncate the
                       file and copy its content. */

                    if (lstat(fpath, &sb) == -1)
                        ERR("lstat");

                    if (S_ISREG(sb.st_mode))
                        try_backup_file(fpath, &sb, dest_fpath);

                    else if (S_ISLNK(sb.st_mode))
                        try_backup_sl(fpath, src_abs, src_abs_len, dest_abs, dest_fpath);

                    /* Other files are ignored. */
                    }

                else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                    if (remove(dest_fpath) == -1)
                        ERR("remove");
                }
            }
            free(fpath);
            free(dest_fpath);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3)
        usage(argv[0]);
    char *src_path = argv[1];
    char *dest_path = argv[2];

    /* Make sure destination directory does not exist, and was empty
       if it did. We must temporairly create the directory to
       calculate its absolute path. */

    if (mkdir(dest_path, 0777) == -1) {
        if (errno != EEXIST)
            ERR("mkdir");
        
        struct stat sb;
        if (stat(dest_path, &sb) == -1)
            ERR("stat");

        DIR *dirp = opendir(dest_path);
        if (dirp == NULL)
            ERR("opendir");
        
        struct dirent *entp;
        while ((entp = readdir(dirp)) != NULL) {
            if (strcmp(entp->d_name, ".") == 0 || strcmp(entp->d_name, "..") == 0)
                continue;
            closedir(dirp);
            usage(argv[0]);
        }
        closedir(dirp);
    }

    if (realpath(src_path, src_abs) == NULL)
        ERR("realpath");
    if (realpath(dest_path, dest_abs) == NULL)
        ERR("realpath");

    if (rmdir(dest_path) == -1)
        ERR("rmdir");
    src_abs_len = strlen(src_abs);

    /* Creating destination directory inside source would cause
       infinite cascade of events. */

    if (src_abs_len <= strlen(dest_abs) &&
        strncmp(src_abs, dest_abs, src_abs_len) == 0 &&
        (dest_abs[src_abs_len] == '/' || dest_abs[src_abs_len] == 0))  {
            usage(argv[0]);
        }
    
    /* Backup the whole tree and add watches. */
    
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1)
        ERR("inotify_init1");
    strvec_init(&inotify_wds);
    at_src = 1;
    nftw(src_path, walk_backup, MAXFD, FTW_PHYS);
    at_src = 0;

    struct pollfd pfd = {inotify_fd, POLLIN};
    int poll_num;
    while (stop_loop == 0) {
        poll_num = poll(&pfd, 1, -1);
        if (poll_num == -1) {
                if (errno == EINTR)
                    continue;
                ERR("poll");
        }
        if (pfd.revents & POLLIN)
            handle_events();
    }
    strvec_free(&inotify_wds);
}
