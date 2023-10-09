#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/***************************** Binary Converters *****************************/

#define BINARY_BYTE "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY(i)    \
    (((i) & 0x80) ? '1' : '0'), \
    (((i) & 0x40) ? '1' : '0'), \
    (((i) & 0x20) ? '1' : '0'), \
    (((i) & 0x10) ? '1' : '0'), \
    (((i) & 0x08) ? '1' : '0'), \
    (((i) & 0x04) ? '1' : '0'), \
    (((i) & 0x02) ? '1' : '0'), \
    (((i) & 0x01) ? '1' : '0')

#define BINARY_2_BYTE \
    BINARY_BYTE     BINARY_BYTE
#define PRINTF_2BYTE_TO_BINARY(i) \
    PRINTF_BYTE_TO_BINARY((i) >> 8),   PRINTF_BYTE_TO_BINARY(i)

#define BINARY_4_BYTE \
    BINARY_2_BYTE   BINARY_2_BYTE
#define PRINTF_4BYTE_TO_BINARY(i) \
    PRINTF_2BYTE_TO_BINARY((i) >> 16), PRINTF_2BYTE_TO_BINARY(i)

/******************************** Main Program *******************************/

void unzip(FILE* fp) {
    char *line = NULL;
    size_t len;

    char c;
    int i, n;
    uint32_t l;

    while(getline(&line, &len, fp) != -1) {
        while (i < len) {
            n = i + 32;
            for (l = 0; i < n; i++) {
                l = 2 * l + (line[i] - '0');
            }
            c = line[i++];
            while (l--) {
                printf("%c", c);
            }
        }
    }

    free(line);
}

#define write(l,c) { \
    fwrite(&l, sizeof(l), 1, stdout); \
    printf("%c", c); \
}

void zip(FILE *fp) {
    char c, new; 
    uint32_t l = 0;

    new = c = fgetc(fp);
    do {
        if (new == c) {
            ++l;
        } else {
            write(l, c);
            c = new;
            l = 1;
        }
    } while ((new = fgetc(fp)) != EOF);
    if (c != '\n') {
        write(l, c);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    FILE *fp;
    
    for (int i = 1; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wzip: invalid file %s", argv[i]);
        }
        zip(fp);
        fclose(fp);
    }

    printf("\n"); 
    return 0;
}