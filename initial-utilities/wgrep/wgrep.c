#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv) {
  FILE *fp;
  char *buffer = NULL;
  size_t buffer_size = 32;

  if (argc <= 1) {
    printf("wgrep: searchterm [file ...]\n");
    exit(EXIT_FAILURE);
  }

  char *search_term = argv[1];
  char *target_file = argv[2];

  if (target_file)
    fp = fopen(target_file, "r");
  else
    fp = stdin;


  if (fp == NULL) {
    printf("wgrep: cannot open file\n");
    exit(EXIT_FAILURE);
  }

  while (getline(&buffer, &buffer_size, fp) != -1)
    if (strstr(buffer, search_term) )
      printf("%s", buffer);

  fclose(fp);
  free(buffer);
  exit(EXIT_SUCCESS);
}
