#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

long get_folder_size(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    long total = 0;

    dir = opendir(path);
    if (dir == NULL) {
        dprintf(STDERR_FILENO, "No access to folder \"%s\"\n", path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        // Skip "." and ".." (current/parent directories)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (lstat(fullpath, &st) == 0) {
            total += st.st_size;
        }
    }

    closedir(dir);
    return total;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc % 2 == 0) {
        dprintf(STDERR_FILENO, "Usage: %s <folder1> <limit1> [<folder2> <limit2> ...]\n", argv[0]);
        return 1;
    }

    // open out.txt for writing (create or truncate)
    int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Cannot open out.txt");
        return 1;
    }

    // Loop through pairs of arguments
    for (int i = 1; i < argc; i += 2) {
        const char *path = argv[i];
        long limit = atol(argv[i + 1]);

        long size = get_folder_size(path);
        if (size >= 0 && size > limit) {
            // Write the folder name to out.txt
            dprintf(fd, "%s\n", path);
        }
    }

    close(fd);
    return 0;
}
