/******************************************************************************
 * Mini Shell in Linux
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: April 2015
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "utils.h"

#define PROMPT "> "

void parse_error(const char *str, const int where) {
  fprintf(stderr, "Parse error near %d: %s\n", where, str);
}

void start_shell() {
  char *line;
  command_t *root;

  int ret;

  for(;;) {
    printf(PROMPT);
    fflush(stdout);
    ret = 0;

    root = NULL;
    line = read_line();
    if (line == NULL) {
      return;
    }
    parse_line(line, &root);

    if (root != NULL) {
      ret = parse_command(root, 0, NULL);
    }

    free_parse_memory();
    free(line);

    if (ret == SHELL_EXIT) {
      break;
    }
  }
}

int main(void) {
  start_shell();

  return EXIT_SUCCESS;
}