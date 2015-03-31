/**
 * Operating Sytems 2013 - Assignment 2
 *
 */

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

#define READ  0
#define WRITE 1

static char *get_word(word_t *s);
static void redirect(simple_command_t *s);

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
  /* TODO execute cd */
  int rc = chdir(get_word(dir));
  return rc;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit()
{
  /* TODO execute exit/quit */

  return 0; /* TODO replace with actual exit code */
}

/**
 * Concatenate parts of the word to obtain the command
 */
static char *get_word(word_t *s)
{
  int string_length = 0;
  int substring_length = 0;

  char *string = NULL;
  char *substring = NULL;

  while (s != NULL) {
    substring = strdup(s->string);

    if (substring == NULL) {
      return NULL;
    }

    if (s->expand == true) {
      char *aux = substring;
      substring = getenv(substring);

      /* prevents strlen from failing */
      if (substring == NULL) {
        substring = calloc(1, sizeof(char));
        if (substring == NULL) {
          free(aux);
          return NULL;
        }
      }

      free(aux);
    }

    substring_length = strlen(substring);

    string = realloc(string, string_length + substring_length + 1);
    if (string == NULL) {
      if (substring != NULL)
        free(substring);
      return NULL;
    }

    memset(string + string_length, 0, substring_length + 1);

    strcat(string, substring);
    string_length += substring_length;

    if (s->expand == false) {
      free(substring);
    }

    s = s->next_part;
  }

  return string;
}

/**
 * Concatenate command arguments in a NULL terminated list in order to pass
 * them directly to execv.
 */
static char **get_argv(simple_command_t *command, int *size)
{
  char **argv;
  word_t *param;

  int argc = 0;
  argv = calloc(argc + 1, sizeof(char *));
  assert(argv != NULL);

  argv[argc] = get_word(command->verb);
  assert(argv[argc] != NULL);

  argc++;

  param = command->params;
  while (param != NULL) {
    argv = realloc(argv, (argc + 1) * sizeof(char *));
    assert(argv != NULL);

    argv[argc] = get_word(param);
    assert(argv[argc] != NULL);

    param = param->next_word;
    argc++;
  }

  argv = realloc(argv, (argc + 1) * sizeof(char *));
  assert(argv != NULL);

  argv[argc] = NULL;
  *size = argc;

  return argv;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_fsimple(simple_command_t *s, int level, command_t *father) {
  /* Sanity checks */
  if (s == NULL) {
    fprintf(stderr, "error parsing simple command: s is NULL\n");
    return -1;
  }
  if (father->cmd1 != NULL) {
    fprintf(stderr, "error parsing simple command: "
      "father->cmd1 is not NULL\n");
    return -2;
  }
  if (father->cmd2 != NULL) {
    fprintf(stderr, "error parsing simple command: "
      "father->cmd2 is not NULL\n");
    return -3;
  }

  /* If builtin command, execute the command */
  char *word = get_word(s->verb);
  if (strcmp(word, "exit") == 0 || strcmp(word, "quit") == 0) {
    int rc = shell_exit();
    exit(rc);
  }

  if (strcmp(word, "cd") == 0) {
    int stdin_copy  = dup(0);
    int stdout_copy = dup(1);
    int stderr_copy = dup(2);
    redirect(s);
    int rc = shell_cd(s->params);
    if (rc < 0) {
      fprintf(stderr, "error cd\n");
      exit(EXIT_FAILURE);
    }
    dup2(stdin_copy,  0);
    dup2(stdout_copy, 1);
    dup2(stderr_copy, 2);
    close(stdin_copy);
    close(stdout_copy);
    close(stderr_copy);
    return rc;
  }

  /* If variable assignment, execute the assignment and return the exit status */
  word_t *next_part = s->verb->next_part;
  if (next_part != NULL && strcmp(next_part->string, "=") == 0) {
    int rc = setenv(s->verb->string, get_word(next_part->next_part), 1);
    if (rc < 0) {
      fprintf(stderr, "error setenv(%s, %s, 0)\n", s->verb->string,
        get_word(next_part->next_part));
    }
    return rc;
  }

  /* External command */
  int pid = fork();
  switch(pid) {
    case -1: {
      fprintf(stderr, "error forking\n");
      return EXIT_FAILURE;
    } case 0: {
      char *cmd = get_word(s->verb);
      int size;
      char **argv = get_argv(s, &size);

      redirect(s);

      execvp(cmd, (char *const *)argv);

      fprintf(stderr, "Execution failed for '%s'\n", cmd);
      exit(EXIT_FAILURE);
    } default: {
      break;
    }
  }

  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    printf("Child %d terminated abnormally, with code %d\n",
      pid, WEXITSTATUS(status));
  }

  return status;
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool do_in_parallel(command_t *cmd1, command_t *cmd2, int level, command_t *father)
{
  /* TODO execute cmd1 and cmd2 simultaneously */

  return true; /* TODO replace with actual exit status */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2)
 */
static bool do_on_pipe(command_t *cmd1, command_t *cmd2, int level, command_t *father)
{
  /* TODO redirect the output of cmd1 to the input of cmd2 */

  return true; /* TODO replace with actual exit status */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father) {
  if (c == NULL) {
    fprintf(stderr, "c is NULL\n");
    exit(EXIT_FAILURE);
  }

  if (c->op == OP_NONE) {
    /* Execute a simple command */
    int rc = parse_fsimple(c->scmd, 0, c);
    return rc;
  }

  switch (c->op) {
    case OP_SEQUENTIAL: {
      /* TODO execute the commands one after the other */
      int rc = parse_command(c->cmd1, level+1, c);
      rc = parse_command(c->cmd2, level+1, c);
      return rc;
    } case OP_PARALLEL: {
      /* TODO execute the commands simultaneously */
      break;
    } case OP_CONDITIONAL_NZERO: {
      int rc = parse_command(c->cmd1, level+1, c);
      if (rc != 0) {
        rc = parse_command(c->cmd2, level+1, c);
      }
      return rc;
    } case OP_CONDITIONAL_ZERO: {
      int rc = parse_command(c->cmd1, level+1, c);
      if (rc == 0) {
        rc = parse_command(c->cmd2, level+1, c);
      }
      return rc;
    } case OP_PIPE: {
      /* TODO redirect the output of the first command to the
       * input of the second */
      break;
    } default: {
      assert(false);
    }
  }

  return 0; /* TODO replace with actual exit code of command */
}

/**
 * Readline from mini-shell.
 */
char *read_line()
{
  char *instr;
  char *chunk;
  char *ret;

  int instr_length;
  int chunk_length;

  int endline = 0;

  instr = NULL;
  instr_length = 0;

  chunk = calloc(CHUNK_SIZE, sizeof(char));
  if (chunk == NULL) {
    fprintf(stderr, ERR_ALLOCATION);
    return instr;
  }

  while (!endline) {
    ret = fgets(chunk, CHUNK_SIZE, stdin);
    if (ret == NULL) {
      break;
    }

    chunk_length = strlen(chunk);
    if (chunk[chunk_length - 1] == '\n') {
      chunk[chunk_length - 1] = 0;
      endline = 1;
    }

    ret = instr;
    instr = realloc(instr, instr_length + CHUNK_SIZE + 1);
    if (instr == NULL) {
      free(ret);
      return instr;
    }
    memset(instr + instr_length, 0, CHUNK_SIZE);
    strcat(instr, chunk);
    instr_length += chunk_length;
  }

  free(chunk);

  return instr;
}

static void redirect(simple_command_t *s) {
  int out_fd;
  if (s->out != NULL) {
    if (s->io_flags == 0) {
      out_fd = open(get_word(s->out), O_WRONLY | O_CREAT | O_TRUNC, 0664);
    } else if (s->io_flags == 1) {
      out_fd = open(get_word(s->out), O_WRONLY | O_APPEND | O_CREAT, 0664);
    } else {
      fprintf(stderr, "flags = %d\n", s->io_flags);
      exit(EXIT_FAILURE);
    }

    if (out_fd < 0) {
      fprintf(stderr, "could not open out file %s\n", get_word(s->out));
      exit(EXIT_FAILURE);
    }

    int rc = dup2(out_fd, STDOUT_FILENO);
    if (rc < 0) {
      fprintf(stderr, "could not dup2\n");
      exit(EXIT_FAILURE);
    }
  }
  
  if (s->err != NULL) {
    int err_fd;
    if (s->io_flags == 0) {
      if (s->out != NULL && strcmp(get_word(s->err), get_word(s->out)) == 0) {
        err_fd = out_fd;
      } else {
        err_fd = open(get_word(s->err), O_WRONLY | O_CREAT | O_TRUNC, 0664);
      }
    } else if (s->io_flags == 1) {
      err_fd = open(get_word(s->err), O_WRONLY | O_APPEND | O_CREAT, 0664);
    } else if (s->io_flags == 2) {
      err_fd = open(get_word(s->err), O_WRONLY | O_APPEND | O_CREAT, 0664);
    } else {
      fprintf(stderr, "flags = %d\n", s->io_flags);
      exit(EXIT_FAILURE);
    }

    if (err_fd < 0) {
      fprintf(stderr, "could not open err file %s\n", get_word(s->err));
      exit(EXIT_FAILURE);
    }

    int rc = dup2(err_fd, STDERR_FILENO);
    if (rc < 0) {
      fprintf(stderr, "could not dup2\n");
      exit(EXIT_FAILURE);
    }
  }

  if (s->in != NULL) {
    int in_fd = open(get_word(s->in), O_RDONLY);
    if (in_fd < 0) {
      fprintf(stderr, "could not open input file %s\n", get_word(s->in));
      exit(EXIT_FAILURE);
    }

    int rc = dup2(in_fd, STDIN_FILENO);
    if (rc < 0) {
      fprintf(stderr, "coudld not dup2\n");
      exit(EXIT_FAILURE);
    }
  }
}
