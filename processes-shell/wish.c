#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/*****************************************************************************/

#define START_SIZE 128

typedef struct _vector {
    char** data;
    int max;
    int size;
} DynamicArray;

DynamicArray initArray() {
    DynamicArray v;
    v.data = (char**) malloc(START_SIZE * sizeof(char*));
    v.max = START_SIZE;
    v.size = 0;
    return v;
}

void arrayInsert(DynamicArray* v, char* val) {
    if (!v || !val) {
        return;
    }

    if (v->max == v->size) {
        char** new_data = (char**) malloc( (v->max << 1) * sizeof(char*));
        int n = v->size;
        for (int i = 0; i < n; i++) {
            new_data[i] = v->data[i];
        }
        free(v->data);
        v->data = new_data;
        v->max <<= 1;
    }
    v->data[v->size++] = val;
}

char* get(DynamicArray* v, int index) {
    if (!v || index < 0 || index >= v->size) {
        return NULL;
    }
    return v->data[index];
}

int getSize(DynamicArray* v) {
    if (!v) {
        return -1;
    }
    return v->size;
}

int findVal(DynamicArray* v, char* val) {
    if (!v || !val) {
        return -1;
    }

    for (int i = 0; i < v->size; i++) {
        if (strcmp(val, v->data[i]) == 0) {
            return i;
        }
    }
    return -1;
}

void freeArray(DynamicArray* v) {
    free(v->data);
}

/*****************************************************************************/

void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

int isDelimiter(char c) {
    return (c == ' ') || (c == '\t') || (c == '>') || (c == '&');
}

char* concat(char* a, char* b) {
    char* ans = (char*) malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(ans, a);
    strcat(ans, b);
    return ans;
}

int hasToken(DynamicArray tokens, char *token) {
    for (int i = 0; i < tokens.size; i++) {
        if (   !strcmp(token, get(&tokens, i))
            && (   (i == 0)
                || (i != tokens.size-1 && !strcmp(token, get(&tokens, i+1))) )
            ) {
                return 0;
            }
    }
    return 1;
}

DynamicArray parseCommands(char* line) {
    DynamicArray ans = initArray();
    int n = strlen(line);
    char* s = NULL;
    int start = -1;
    for (int i = 0; i < n; i++) {
        if (line[i] == '>') {
            arrayInsert(&ans, ">");
            continue;
        }
        if (line[i] == '&') {
            arrayInsert(&ans, "&");
            continue;
        }
        if (!isDelimiter(line[i])) {
            if (i == 0 || isDelimiter(line[i-1])) start = i;
            if (i == n-1 || isDelimiter(line[i+1])) {
                s = strndup(line+start, i-start+1);
                arrayInsert(&ans, s);
            }
        }
    }
    return ans;
}

/*****************************************************************************/

int executeCommand(DynamicArray tokens) {
    char* command = get(&tokens, 0);
    if (strcmp(command, "exit") == 0) {
        if (tokens.size != 1) {
            printError();
        } else {
            exit(0);
        }
        return -1;
    }
    if (strcmp(command, "cd") == 0) {
        if (tokens.size != 2) {
            printError();
        } else {
            chdir(get(&tokens, 1));
        }
        return -1;
    }
    if (tokens.size >= 1 && strcmp("path", command) == 0) {
        DynamicArray params = initArray();
        for (int i = 1; i < tokens.size; i++) {
            arrayInsert(&params, get(&tokens, i));
        }
        PATH = params;
        return -1;
    }
    
    int pos = findVal(&tokens, ">");
    if (pos == -1) {
        pos = tokens.size;
    }
    for (int i = 0; i < PATH.size; i++) {
        char* p = concat(get(&PATH, i), concat("/", command));
        if (access(p, X_OK) == 0) {
            char* argv[pos+1];
            for (int i = 0; i < pos; i++) {
                argv[i] = get(&tokens, i);
            }
            argv[pos] = NULL;
            
            int pid = fork();
            if (pid == 0) {
                if (pos != tokens.size) {
                    close(STDOUT_FILENO);
                    open(get(&tokens, tokens.size - 1), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                }
                execv(p, argv);
            }
            return pid;
        }
    }

    printError();
    return -1;
}

/*****************************************************************************/


DynamicArray PATH;

int main (int argc, char **argv) {
    FILE* source = stdin;
    if (argc > 2) {
        printError();
        exit(1);
    } else if (argc == 2) {
        source = fopen(argv[1], "r");
        if (!source) {
            printError();
            exit(1);
        }
    }

    arrayInsert(&PATH, "/bin");

    while (1) {
        if (argc == 1) {
            printf("wish> ");
        }

        char* line = NULL;
        size_t n = 0;
        int len = getline(&line, &n, source);
        if (len == -1) {
            if (argc == 1) {
                continue;
            }
            break;  // end of shell script
        }
        line[len-1] = '\0';

        DynamicArray tokens = parseCommands(line);
        DynamicArray command = initArray();
        int num_commands = 1, sz = 0;
        int processIds[num_commands];

        if (!hasToken(tokens, "&")) {
            printError();
            continue;
        }
        for (int i = 0; i < tokens.size; i++) {
            if (strcmp("&", get(&tokens, i)) == 0) {
                num_commands++;
            }
        }
        for (int i = 0; i < tokens.size; i++) {
            if (strcmp("&", get(&tokens, i)) != 0) {
                arrayInsert(&command, get(&tokens, i));
            } else {
                continue;
            }
            
            if ((i == tokens.size-1) || !strcmp("&", get(&tokens, i+1))) {
                if (!hasToken(command, ">")) {
                    printError();
                    break;
                }
                int res = executeCommand(command);
                if (res != -1) {
                    processIds[sz++] = res;
                }
                command = initArray();
            }
        }

        for (int i = 0; i < sz; i++) {
            waitpid(processIds[i], NULL, 0);
        }

        free(line);
        freeArray(&tokens);
        freeArray(&command);
    }

    return 0;
}