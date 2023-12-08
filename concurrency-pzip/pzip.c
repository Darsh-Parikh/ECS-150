/**
 * * file reading using mmap: https://www.lemoda.net/c/mmap-example/
*/

#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include <pthread.h>


//-----------------------------------------------------------------------------
// Utilites

#define __DEBUG

struct _file {
    int fd;
    size_t filesize_bytes;
};
typedef struct _file mFile_t;

size_t fileSize(int fd) {
   struct stat s;
   if (fstat(fd, &s) == -1) {
      fprintf(stderr, "fstat(%d) failed", fd);
      return 0;
   }
   return (size_t)s.st_size;
}


struct _work_list {
    struct _work_list *next;
    char character;
    size_t count;
};
typedef struct _work_list WorkList;

//-----------------------------------------------------------------------------
// Worker System

struct _worker {
    int id;
    void *addr;
    size_t num_bytes;
    WorkList *work_start;
    WorkList *work_end;
};
typedef struct _worker Worker;


// Worker Functions

void worker_clearwork(Worker *worker) {
    WorkList *curr = worker->work_start, *next;
    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    worker->work_start = worker->work_end = NULL;
}

void worker_addwork(Worker *worker, WorkList *work) {
    if (!worker || !work) {
        return;
    }
    if (worker->work_start) {
        work->next = worker->work_end->next;
        worker->work_end = work;
    } else {
        worker->work_start = worker->work_end = work;
        work->next = NULL;
    }

#ifdef __DEBUG
    fprintf(stdout, "Worker:%d working on %ld bytes from %p : Found %ld occurences of %c\n", worker->id, worker->num_bytes, worker->addr, work->count, work->character);
#endif

}

Worker* new_worker() {
    Worker *w = malloc(sizeof(Worker));
    if (w) {
        w->addr = w->work_end = w->work_start = NULL;
        w->num_bytes = 0;
    }
    return w;
}

void fire_worker(Worker *worker) {
    if (!worker) {
        return;
    }
    worker_clearwork(worker);
    free(worker);
}

static void* Job(void *worker) {
    Worker *_worker = (Worker*)worker;

    //! For Testing only: REMOVE THIS
    // wait for contractor
    // inform contractor
    return NULL;


    // TODO: Wait for Contractor to Inform Worker

    // TODO: Erorr Conditions???

    char *buffer = _worker->addr;
    char tracked_char = *buffer;
    size_t count = 0;
    for (size_t i = 0; i < _worker->num_bytes; i++) {
        if (buffer[i] == tracked_char) {
            count++;
        } else {
            WorkList *new_work = malloc(sizeof(WorkList));
            new_work->character = tracked_char;
            new_work->count = count;
            worker_addwork(worker, new_work);
        }
    }
    WorkList *new_work = malloc(sizeof(WorkList));
    new_work->character = tracked_char;
    new_work->count = count;
    worker_addwork(worker, new_work);

    // TODO: Inform Contractor about being done

    return NULL;
}


//-----------------------------------------------------------------------------

static pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pzip file1 [file2 ...]\n");
        return EXIT_FAILURE;
    }
    mFile_t *files = malloc(sizeof(mFile_t) * (argc-1));    // for saving file data
    if (!files) {
        fprintf(stderr, "ERROR: Not enough space to track initial file data\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "ERROR: input file ( %s ) cannot be opened\n", argv[i]);
            free(files);
            exit(EXIT_FAILURE);
        }
        
        size_t size = fileSize(fd);
        if (!size) {
            exit(EXIT_FAILURE);
        }

        files[i-1].filesize_bytes = size;
        files[i-1].fd = fd;

#ifdef __DEBUG
        fprintf(stdout, "File:%s , Size:%ld\n", argv[i], files[i-1].filesize_bytes);
#endif

    }

    int num_workers = 2;
    //TODO: int num_workers = get_nprocs(); // get # of available processors
    pthread_t *threads = malloc(sizeof(pthread_t) * num_workers);
    Worker *workers = malloc(sizeof(Worker) * num_workers);
    if (!threads || !workers) {
        fprintf(stderr, "ERROR: Not enough space to utilize parallelism\n");
        free(files);
        exit(EXIT_FAILURE);
    }

#ifdef __DEBUG
    fprintf(stdout, "Created %d workers given the system constrains\n", num_workers);
#endif
    
    for (int i = 0; i < argc-1; i++) {
        // TODO: Map file

        // TODO: Break File into Chunks

        pthread_mutex_lock(&work_lock);
        // TODO: Assign each worker their work
        pthread_mutex_unlock(&work_lock);

        // TODO: Wait for workers to be done
        // wait for all the threads to join...

        // TODO: unmap the file

        // TODO: go through work_lists, merge adjecent IF same character

        // TODO: go through lists, print result

        // TODO: clear lists

    }

    free(files);
    free(threads);

    return EXIT_SUCCESS;
}




//-----------------------------------------------------------------------------

int zip(int argc, char *argv[]) {
  FILE *file;
  char character[2] = "";
  char previous_character[2] = "";
  int char_occurrences = 0;

  for (int i=1; i < argc; i++) {
    char *file_name = argv[i];
    if ((file = fopen(file_name, "r")) == NULL) {
      fprintf(stderr, "wzip: cannot open file");
      exit(EXIT_FAILURE);
    }

    while (fread(&character, 1, 1, file)) {
      if (strcmp(character, previous_character) == 0) {
        char_occurrences++;
      } else {
        if (previous_character[0] != '\0') {
          fwrite(&char_occurrences, 4, 1, stdout);
          fwrite(previous_character, 1, 1, stdout);
        }
        char_occurrences = 1;
        strcpy(previous_character, character);
      }
    }
    fclose(file);
  }

  fwrite(&char_occurrences, 4, 1, stdout);
  fwrite(previous_character, 1, 1, stdout);
  return 1;
}
