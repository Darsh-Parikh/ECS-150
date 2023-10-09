#include <stdio.h>

#define BUFF_LEN 1024   // lots of characters

int main(int argc, char **argv) {
    int num_files = argc - 1;
    if (!num_files) {
        return 0;
    }

    for (int i = 1; i <= num_files; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            printf("wcat: cannot open file %s\n", argv[i]);
            return 1;
        }

        char buffer[BUFF_LEN];
        while (fgets(buffer, BUFF_LEN, fp)) {                                  //? Can I use fgetc ????
            printf("%s", buffer);
        }

        // printf("\n");
        fclose(fp);
    }
    return 0;
}