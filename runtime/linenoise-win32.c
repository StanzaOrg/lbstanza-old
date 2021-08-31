#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

void linenoiseSetMultiLine(int b) {
  /* stub */
}

char* getline(void) {
  char* buffer = NULL;
  size_t chars_read = 0;
  size_t buffer_size = 0;

  while (true) {
    int c = fgetc(stdin);
    if (c == EOF || c == '\n') {
      break;
    }
    if (buffer == NULL) {
      buffer = malloc(256);
      buffer_size = 256;
    }
    if (chars_read >= buffer_size) {
      buffer_size += 256;
      buffer = realloc(buffer, buffer_size);
    }
    buffer[chars_read] = (char) c;
    chars_read++;
  }
  buffer[chars_read] = '\0';

  return buffer;
}

char* linenoise(const char* prompt) {
  fflush(stdout);
  fputs(prompt, stdout);
  fflush(stdout);

  return getline();
}

int linenoiseHistoryAdd(const char *line) {
  /* stub */
  return -1;
}
