#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

int histogram[256]; // Global variable

typedef struct {
    char ch;
    int count;
} CharCount;

// Comparison function for qsort (descending by count, ascending by ASCII)
int qsort_comparison(const void *a, const void *b) {
    CharCount *ca = (CharCount *)a;
    CharCount *cb = (CharCount *)b;

    if (cb->count != ca->count)
        return cb->count - ca->count;
    return (unsigned char)ca->ch - (unsigned char)cb->ch;
}

// ----------------------------- ANALYZE --------------------------------
void analyze_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    memset(histogram, 0, sizeof(histogram));

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        histogram[(unsigned char)ch]++;
    }
    fclose(file);

    // Count unique characters
    int unique_count = 0;
    for (int i = 0; i < 256; i++)
        if (histogram[i] > 0)
            unique_count++;

    // Ensure ./output exists
    mkdir("./output", 0777);

    FILE *out = fopen("./output/analysis", "wb");
    if (!out) {
        perror("Error creating output file");
        return;
    }

    // Write number of unique characters
    fwrite(&unique_count, sizeof(int), 1, out);

    // Write characters
    for (int i = 0; i < 256; i++)
        if (histogram[i] > 0)
            fputc(i, out);

    // Write counts
    for (int i = 0; i < 256; i++)
        if (histogram[i] > 0)
            fwrite(&histogram[i], sizeof(int), 1, out);

    fclose(out);
    printf("File analyzed and intermediate cipher saved to ./output/analysis\n");
}

// ----------------------------- DECODE --------------------------------
void decode_message(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening file");
        return;
    }

    int unique_count;
    if (fread(&unique_count, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Error reading header\n");
        fclose(file);
        return;
    }

    char *chars = malloc(unique_count);
    int *counts = malloc(unique_count * sizeof(int));

    fread(chars, 1, unique_count, file);
    fread(counts, sizeof(int), unique_count, file);
    fclose(file);

    CharCount *data = malloc(unique_count * sizeof(CharCount));
    for (int i = 0; i < unique_count; i++) {
        data[i].ch = chars[i];
        data[i].count = counts[i];
    }

    qsort(data, unique_count, sizeof(CharCount), qsort_comparison);

    printf("Decoded message:\n");
    for (int i = 0; i < unique_count; i++) {
        for (int j = 0; j < data[i].count; j++)
            putchar(data[i].ch);
    }
    printf("\n");

    free(chars);
    free(counts);
    free(data);
}

// ----------------------------- BATCH --------------------------------
void batch_decode(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Cannot open directory");
        return;
    }

    mkdir("./output", 0777);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (lstat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            printf("Decoding %s...\n", filepath);

            // Build output filename
            char outpath[1024];
            snprintf(outpath, sizeof(outpath), "./output/%s", entry->d_name);

            FILE *in = fopen(filepath, "rb");
            FILE *out = fopen(outpath, "w");
            if (!in || !out) {
                perror("Error opening file");
                if (in) fclose(in);
                if (out) fclose(out);
                continue;
            }

            int unique_count;
            fread(&unique_count, sizeof(int), 1, in);

            char *chars = malloc(unique_count);
            int *counts = malloc(unique_count * sizeof(int));

            fread(chars, 1, unique_count, in);
            fread(counts, sizeof(int), unique_count, in);
            fclose(in);

            for (int i = 0; i < unique_count; i++) {
                for (int j = 0; j < counts[i]; j++)
                    fputc(chars[i], out);
            }

            fclose(out);
            free(chars);
            free(counts);

            printf("Saved decoded file to %s\n", outpath);
        }
    }

    closedir(dir);
}

// ----------------------------- MAIN --------------------------------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <command> <path>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *path = argv[2];
    struct stat st;

    if (lstat(path, &st) != 0) {
        perror("Error accessing path");
        return 1;
    }

    if (strcmp(command, "analyze") == 0) {
        if (S_ISREG(st.st_mode) || (S_ISLNK(st.st_mode) && access(path, F_OK) == 0))
            analyze_file(path);
        else
            fprintf(stderr, "Error: 'analyze' expects a regular file.\n");
    } else if (strcmp(command, "decode") == 0) {
        if (S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
            decode_message(path);
        else
            fprintf(stderr, "Error: 'decode' expects a regular file (not link).\n");
    } else if (strcmp(command, "batch") == 0) {
        if (S_ISDIR(st.st_mode))
            batch_decode(path);
        else
            fprintf(stderr, "Error: 'batch' expects a directory.\n");
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
    }

    return 0;
}
