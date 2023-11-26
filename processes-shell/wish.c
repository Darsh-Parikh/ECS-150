#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


#define START_SIZE 128

typedef struct _vector {
    char** data;
    int capacity;
    int size;
} DynamicArray;

DynamicArray initArray() {
    DynamicArray v;
    v.data = (char**) malloc(START_SIZE * sizeof(char*));
    v.capacity = START_SIZE;
    v.size = 0;
    return v;
}

void arrayInsert(DynamicArray* v, char* val) {
    if (v->capacity == v->size) {
        char** new_data = (char**) malloc( (v->capacity << 1) * sizeof(char*));
        int n = v->size;
        for (int i = 0; i < n; i++) {
            new_data[i] = v->data[i];
        }
        free(v->data);
        v->data = new_data;
        v->capacity <<= 1;
    }
    v->data[v->size++] = val;
}

char* get(DynamicArray* v, int index) {
    if (index < 0 || index >= v->size) return NULL;
    return v->data[index];
}

int getSize(DynamicArray* v) {
    return v->size;
}

int findVal(DynamicArray* v, char* val) {
    int n = v->size;
    for (int i = 0; i < n; i++) {
        if (strcmp(val, v->data[i]) == 0) {
            return i;
        }
    }
    return -1;
}

void freeArray(DynamicArray* v) {
    free(v->data);
}



/* Given a command line, return a vector of logical tokens */
DynamicArray parseCommands(char* line);

/* Returns true of both of the file descriptors point to the same file */
int isSameFile(int, int);

/* check if the given character should be used as a delimeter for parseCommands() */
int isDelimiter(char);

/* Returns true if the given command is valid in terms of the redirection syntax */
int isValidRedirection(DynamicArray tokens);

/* Returns true if the given command is valid in terms of the Ampersand syntax */
int isValidAmpersand(DynamicArray tokens);

/* Given two strings, return the concatenation of them */
char* concat (char* a, char* b);

/* Output the one and only error message to the screen */
void printError ();



/**
 * Given a command as a vector of tokens, execute the command
 * Returns the process id of the created process
 */
int executeCommand (DynamicArray tokens);


DynamicArray PATH;

int main (int argc, char **argv) {
    FILE* input_file = NULL;
    for (int i = 1; i < argc; i++) {
        FILE* cur_file = fopen(argv[i], "r");
        if (cur_file == NULL) {
            printError();
            exit(1);
        }

        if (input_file == NULL) {
            input_file = cur_file;
        } else if (!isSameFile(fileno(input_file), fileno(cur_file))) {
            printError();
            exit(1);
        }
    }

    if (input_file != NULL) {
        stdin = fdopen(fileno(input_file), "r");
    }

    arrayInsert(&PATH, "/bin");

    while (1) {
        if (input_file == NULL) {
            printf("wish> ");
        }

        char* line = NULL;
        size_t n = 0;
        int len = getline(&line, &n, stdin);
        if (len == -1) {
            if (input_file == NULL) continue;
            break;
        }
        line[len-1] = '\0';

        DynamicArray tokens = parseCommands(line);
        if (!isValidAmpersand(tokens)) {
            continue;
        }
        int commandsCount = 1;
        for (int i = 0; i < tokens.size; i++) {
            if (strcmp("&", get(&tokens, i)) == 0) commandsCount++;
        }
        DynamicArray command = initArray();
        int processIds[commandsCount];
        int sz = 0;
        for (int i = 0; i < tokens.size; i++) {
            if (strcmp("&", get(&tokens, i)) != 0) {
                arrayInsert(&command, get(&tokens, i));
            } else {
                continue;
            }
            
            if (i == tokens.size - 1 || strcmp("&", get(&tokens, i+1)) == 0) {
                if (!isValidRedirection(command)) {
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

int isDelimiter(char c) {
    return (c == ' ') || (c == '\t') || (c == '>') || (c == '&');
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

int isSameFile(int fd1, int fd2) {
    struct stat stat1, stat2;
    if(fstat(fd1, &stat1) < 0) return -1;
    if(fstat(fd2, &stat2) < 0) return -1;
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

int isValidRedirection(DynamicArray tokens) {
    int n = tokens.size;
    for (int i = 0; i < n; i++) {
        if (strcmp(">", get(&tokens, i)) == 0) {
            if (i == 0 || i == n-1 || n-1-i > 1) return 0;
            if (i != n-1 && strcmp(">", get(&tokens, i+1)) == 0) return 0;
        }
    }
    return 1;
}

int isValidAmpersand(DynamicArray tokens) {
    int n = tokens.size;
    for (int i = 0; i < n; i++) {
        if (strcmp("&", get(&tokens, i)) == 0) {
            if (i == 0) return 0;
            if (i != n-1 && strcmp("&", get(&tokens, i+1)) == 0) return 0;
        }
    }
    return 1;
}

char* concat(char* a, char* b) {
    char* ans = (char*) malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(ans, a);
    strcat(ans, b);
    return ans;
}

void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

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
    
    /**
     * Spawn the required process to execute the command
     * Supports redirection to files
     */
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
            
            int rc = fork();
            if (rc == 0) {
                if (pos != tokens.size) {
                    close(STDOUT_FILENO);
                    open(get(&tokens, tokens.size - 1), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                }
                execv(p, argv);
            }
            return rc;
        }
    }

    printError();
    return -1;
}