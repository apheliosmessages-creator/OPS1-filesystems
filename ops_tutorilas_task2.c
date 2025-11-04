#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void scan_directory(const char *path, int current_depth, int max_depth, const char *ext, FILE *out) {
    if (current_depth > max_depth) return;

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", path);
        return;
    }

    fprintf(out, "path: %s\n", path);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (lstat(fullpath, &st) != 0)
            continue;

        if (S_ISLNK(st.st_mode))
            continue;

        if (S_ISDIR(st.st_mode)) {
            scan_directory(fullpath, current_depth + 1, max_depth, ext, out);
        } else if (S_ISREG(st.st_mode)) {
            if (!ext) {
                fprintf(out, "%s %ld\n", entry->d_name, st.st_size);
            } else {
                const char *dot = strrchr(entry->d_name, '.');
                if (dot && strcmp(dot + 1, ext) == 0) {
                    fprintf(out, "%s %ld\n", entry->d_name, st.st_size);
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    char *ext = NULL;
    int depth = 1;
    int output_to_file = 0;
    FILE *out = stdout;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            depth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            ext = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0) {
            output_to_file = 1;
        }
    }

    // Handle output redirection
    if (output_to_file) {
        char *outputfile = getenv("L1_OUTPUTFILE");
        if (outputfile && strlen(outputfile) > 0) {
            out = fopen(outputfile, "w");
            if (!out) {
                perror("Cannot open output file");
                out = stdout;
            }
        }
    }

    // Scan directories (-p flags)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            scan_directory(argv[++i], 1, depth, ext, out);
        }
    }

    if (out != stdout) fclose(out);
    return 0;
}
