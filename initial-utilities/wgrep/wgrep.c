#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! IS FAULTY ON FILE INPUTS

int findword(char *buff, char *word) {
    if (!buff || !word) {
        return -1;
    }

    size_t size;
    if((size = strlen(word)) == 0) {
        return 0;
    }
    size -= 1;

    int b_idx = 0;
    int w_idx = 0;
    char c;
    while ((c = buff[b_idx++]) != '\0') {
        if (w_idx == size) {
            return 1;
        }
        if (c == word[w_idx]) {
            w_idx++;
        } else {
            w_idx = 0;
        }
    }
    return 0;
}

void wgrep(char *word, FILE *fp) {
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fp) != -1) {
        if (findword(line, word) > 0) {
            printf("%s", line);
        }
    }
    free(line);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("wgrep: searchterm [file...]\n");
        return 1;
    }

    if (argc == 2) {
        wgrep(argv[1], stdin);
        return 0;
    }
    
    FILE *fp;
    for (int i = 2; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wgrep: cannot open file %s", argv[i]);
            return 1;
        }
        wgrep(argv[1], fp);
        fclose(fp);
    }

    return 0;
}