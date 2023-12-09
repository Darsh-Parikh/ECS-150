#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>          // open, O_* constants
#include <sys/sysinfo.h>    // get_nproc()
#include <sys/mman.h>       // mmap, munmap
#include <sys/stat.h>       // fstat
#include <unistd.h>         // sysconf, close

#include <pthread.h>
#include <semaphore.h>

//-----------------------------------------------------------------------------

// #define __DEBUG

void write_out(int count, char *character) {
    fwrite(&count, 4, 1, stdout);
    fwrite(character, 1, 1, stdout);
}

//-----------------------------------------------------------------------------

typedef struct _work_list {
    struct _work_list *next;
    int count;
    char character;
} WorkList;

WorkList *work_list_block(int count, char character) {
    WorkList *work_block = malloc(sizeof(WorkList));
    if (!work_block) {
        fprintf(stderr, "ERROR: Cannot create new work block\n");
        exit(EXIT_FAILURE);
    }
    work_block->count = count;
    work_block->character = character;
    work_block->next = NULL;
    return work_block;
}

//-----------------------------------------------------------------------------

typedef struct _work {
    size_t chunk_size;
    char *addr;
    WorkList *work;
} Work;

typedef struct _file {
    int fd;
    off_t size;
} mFile_t;

//-----------------------------------------------------------------------------

size_t chunkInUse_idx = 0, chunkFilled_idx = 0, chunks = 0;
sem_t mutex, worker_waiting, worker_loaded;

//-----------------------------------------------------------------------------

void *Job(void *arg) {
    Work *workChunk_list = (Work*)arg;

    while (1) {
        // wait to be assigned work
        sem_wait(&worker_loaded);
        sem_wait(&mutex);

        // get work
        Work *curr_worker = &workChunk_list[chunkInUse_idx];
        chunkInUse_idx = (chunkInUse_idx + 1) % chunks;

        // do work
        WorkList *head = NULL, *prev = NULL;
        char tracked_char = '\0';
        int count = 0;
        for (size_t i = 0; i < curr_worker->chunk_size; i++) {
            char c = curr_worker->addr[i];
            if (c == tracked_char) {
                count++;
            } else {
                if (count != 0) {
                    WorkList *last_work_block = work_list_block(count, tracked_char);
                    if (prev) {
                        prev->next = last_work_block;
                    }
                    prev = last_work_block;
                    if (!head) {
                        head = prev;
                    }
                }
                tracked_char = c;
                count = 1;
            }
        }
        if (!head) {    // Only 1 type of Character
            curr_worker->work = work_list_block(count, tracked_char);
        } else {
            prev->next = work_list_block(count, tracked_char);
            curr_worker->work = head;
        }

        // inform contractor about completion
        sem_post(&mutex);
        sem_post(&worker_waiting);
    }

    return NULL;
}

//-----------------------------------------------------------------------------

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stdout, "pzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    mFile_t *files = malloc(sizeof(mFile_t) * (argc - 1));
    if (!files) {
        fprintf(stderr, "ERROR: Not enough space for tracking input files\n");
        exit(EXIT_FAILURE);
    }

    size_t page_size = sysconf(_SC_PAGE_SIZE);
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "ERROR: Invalid file (%s) encountered\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            fprintf(stderr, "ERROR: fstat failure on file %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }

        files[i-1].fd = fd;
        files[i-1].size = sb.st_size;

        chunks += (sb.st_size / page_size) + 1;
    }

    // get_nprocs is GNU extension
    int num_threads = get_nprocs();
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    if (!threads) {
        fprintf(stderr, "ERROR: Not enough space for storing thread info\n");
        exit(EXIT_FAILURE);
    }

    sem_init(&mutex, 0, 1);
    sem_init(&worker_waiting, 0, 1);   // Prevents the main thread stopping other threads prematurely
    sem_init(&worker_loaded, 0, 0);

    Work *workChunk_list = malloc(sizeof(Work) * chunks);
    if (workChunk_list == NULL) {
        fprintf(stderr, "ERROR: Not enough space for tracking work\n");
        exit(EXIT_FAILURE);
    }

    // create workers
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, Job, workChunk_list);
    }

    // create jobs
    for (int i = 0; i < argc - 1; i++) {
        size_t offset = 0;
        while (offset < (size_t)files[i].size) {
            sem_wait(&worker_waiting);
            sem_wait(&mutex);

            workChunk_list[chunkFilled_idx].chunk_size = page_size;
            if ((page_size + offset) > (size_t)files[i].size) {
                workChunk_list[chunkFilled_idx].chunk_size = files[i].size - offset;
            }

            char *addr = mmap(NULL, workChunk_list[chunkFilled_idx].chunk_size, PROT_READ, MAP_PRIVATE, files[i].fd, offset);
            if (addr == MAP_FAILED) {
                fprintf(stderr, "ERROR: Cannot mmap chunks %ld bytes in from file: %s\n", offset, argv[i]);
                exit(EXIT_FAILURE);
            }

            workChunk_list[chunkFilled_idx].addr = addr;
            workChunk_list[chunkFilled_idx].work = NULL;

            offset += page_size;
            chunkFilled_idx = (chunkFilled_idx + 1) % chunks;

            sem_post(&mutex);
            sem_post(&worker_loaded);
        }
        close(files[i].fd);
    }

    sem_wait(&worker_waiting);
    sem_wait(&mutex);

    // kill workers
    for (int i = 0; i < num_threads; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    free(threads);
    free(files);

    // output all the work
    int last_count = 0;
    char last_character = '\0';
    for (size_t i = 0; i < chunks; i++) {
        WorkList *work_block = workChunk_list[i].work, *tmp;
        while (work_block) {
            if (work_block == workChunk_list[i].work && work_block->next) {
                if (work_block->character == last_character) {
                    write_out(work_block->count + last_count, &work_block->character);
                } else {
                    if (last_count > 0) {
                        write_out(last_count, &last_character);
                    }
                    write_out(work_block->count, &work_block->character);
                }
            } else if (!work_block->next) {
                if (work_block != workChunk_list[i].work) {
                    if (i == chunks - 1) {
                        write_out(work_block->count, &work_block->character);
                    } else {
                        last_character = work_block->character;
                        last_count = work_block->count;
                    }
                } else {
                    if (work_block->character == last_character) {
                        if (i != chunks - 1) {
                            last_count += work_block->count;
                        } else {
                            write_out(work_block->count + last_count, &work_block->character);
                        }
                    } else { // not same
                        if (last_count > 0) {
                            write_out(last_count, &last_character);
                        }
                        if (i != chunks - 1) {
                            last_character = work_block->character;
                            last_count = work_block->count;
                        } else {
                            write_out(work_block->count, &work_block->character);
                        }
                    }
                }
            } else {
                write_out(work_block->count, &work_block->character);
            }

            tmp = work_block;
            work_block = work_block->next;
            free(tmp);
        }
        if (munmap(workChunk_list[i].addr, (size_t)workChunk_list[i].chunk_size) != 0) {
            fprintf(stderr, "ERROR: munmap issues\n");
            exit(EXIT_FAILURE);
        }
    }

    free(workChunk_list);

    sem_destroy(&mutex);
    sem_destroy(&worker_loaded);
    sem_destroy(&worker_waiting);

    return 0;
}
