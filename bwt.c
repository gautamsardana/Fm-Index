#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "suffix_array.h"

char *build_bwt(const char *s, int *sa, int n) {
    char *bwt = malloc((n + 1) * sizeof(char));
    for (int i = 0; i < n; i++) {
        bwt[i] = s[(sa[i] - 1 + n) % n];
    }
    bwt[n] = '\0';
    return bwt;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./bwt <input_file>\n");
        return 1;
    }

    // read input file
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Error: cannot open file %s\n", argv[1]);
        return 1;
    }

    // get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // read text and append sentinel
    char *text = malloc(fsize + 2);
    fread(text, 1, fsize, f);
    fclose(f);

    // strip newline if present
    int len = strlen(text);
    while (len > 0 && (text[len-1] == '\n' || text[len-1] == '\r')) {
        text[--len] = '\0';
    }

    // append sentinel '$'
    text[len] = '$';
    text[len + 1] = '\0';
    int n = len + 1;

    printf("Text: %s\n", text);

    int *sa = build_suffix_array(text, n);

    printf("Suffix Array: ");
    for (int i = 0; i < n; i++) printf("%d ", sa[i]);
    printf("\n");

    char *bwt = build_bwt(text, sa, n);
    printf("BWT: %s\n", bwt);

    free(sa);
    free(bwt);
    free(text);
    return 0;
}