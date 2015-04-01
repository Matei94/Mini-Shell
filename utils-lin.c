/******************************************************************************
 * Mini Shell in Linux - API implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: April 2015
 *****************************************************************************/

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "minternals.h"
#include "utils.h"



/* Declarations */
static int  shell_exit ();
static bool shell_cd   (word_t *dir);

static int  do_simple     (simple_command_t *s, int level, 
                           command_t *father);
static int  do_in_parallel(command_t *cmd1, command_t *cmd2, int level, 
                           command_t *father);
static bool do_on_pipe    (command_t *cmd1, command_t *cmd2, int level,
                           command_t *father);

static void redirect_all (simple_command_t *s);
static void redirect_in  (simple_command_t *s);
static void redirect_out (simple_command_t *s);
static void redirect_err (simple_command_t *s);

static char  *get_word (word_t *s);
static char **get_argv (simple_command_t *command, int *size);



/**
 * Readline from mini-shell.
 */
char *read_line() {
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

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father) {
  if (c == NULL) {
    mfatal("c is NULL");
  }

  if (c->op == OP_NONE) {
    /* Execute a simple command */
    int rc = do_simple(c->scmd, level+1, c);
    return rc;
  }

  switch (c->op) {
    case OP_SEQUENTIAL: {
      /* Execute the commands one after the other */
      int rc = parse_command(c->cmd1, level+1, c);
      rc = parse_command(c->cmd2, level+1, c);
      return rc;
    } case OP_PARALLEL: {
      /* Execute the commands simultaneously */
      int rc = do_in_parallel(c->cmd1, c->cmd2, level+1, c);
      return rc;
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
      int rc = do_on_pipe(c->cmd1, c->cmd2, level+1, c);
      return rc;
    } default: {
      assert(false);
    }
  }

  return EXIT_FAILURE;
}



/**
 * Internal exit/quit command.
 */
static int shell_exit() {
  return SHELL_EXIT;
}

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir) {
  char *dir_name = get_word(dir);
  int rc = chdir(dir_name);
  free(dir_name);
  if (rc != 0) {
    perror("Could not change directory");
    exit(EXIT_FAILURE);
  }
  return rc;
}

/**
 * Execute a simple command (internal, environment variable assignment,
 * external command).
 */
static int do_simple(simple_command_t *s, int level, command_t *father) {
  /* Sanity checks */
  if (s == NULL) {
    mfatal("s is NULL");
  }
  if (father->cmd1 != NULL) {
    mfatal("cmd1 not NULL");
  }
  if (father->cmd2 != NULL) {
    mfatal("cmd2 not NULL");
  }

  /* If builtin command, execute the command */
  char *word = get_word(s->verb);
  if (strcmp(word, "exit") == 0 || strcmp(word, "quit") == 0) {
    free(word);
    return shell_exit();
  }

  if (strcmp(word, "cd") == 0) {
    /* Save context */
    int stdin_copy  = dup(0);
    int stdout_copy = dup(1);
    int stderr_copy = dup(2);

    redirect_all(s);
    int rc = shell_cd(s->params);

    /* Restore context */
    dup2(stdin_copy,  0);
    dup2(stdout_copy, 1);
    dup2(stderr_copy, 2);
    close(stdin_copy);
    close(stdout_copy);
    close(stderr_copy);

    free(word);
    return rc;
  }

  /* If variable assignment, execute the assignment */
  word_t *next_part = s->verb->next_part;
  if (next_part != NULL && strcmp(next_part->string, "=") == 0) {
    /* Add or overwrite if exists */
    char *value = get_word(next_part->next_part);
    int rc = setenv(s->verb->string, value, 1);
    if (rc < 0) {
       perror("Could not set environment variable");
       exit(EXIT_FAILURE);
    }

    free(value);
    free(word);
    return rc;
  }

  /* External command */
  int pid = fork();
  switch(pid) {
    case -1: { /* Fork error */
      perror("Could not fork");
      free(word);
      exit(EXIT_FAILURE);
    } case 0: { /* Child */
      char *cmd = get_word(s->verb);
      int size;
      char **argv = get_argv(s, &size);

      redirect_all(s);

      execvp(cmd, (char *const *)argv);

      fprintf(stderr, "Execution failed for '%s'\n", cmd);
      exit(EXIT_FAILURE);
    } default: { /* Parent */
      break;
    }
  }

  /* Wait for child */
  int status;
  waitpid(pid, &status, 0);

  free(word);
  return status;
}

/**
 * Process two commands in parallel, by creating two children.
 */
static int do_in_parallel(command_t *cmd1, command_t *cmd2, int level,
    command_t *father) {
  /* First command */
  int pid1 = fork();
  switch(pid1) {
    case -1: { /* Fork error */
      perror("Could not fork");
      exit(EXIT_FAILURE);
    } case 0: { /* Child */
      int rc = parse_command(cmd1, level + 1, father);
      exit(rc);
    } default: { /* Parent */
      break;
    }
  }

  /* Second command */
  int pid2 = fork();
  switch(pid2) {
    case -1: {/* Fork error */
      perror("Could not fork");
      exit(EXIT_FAILURE);
    } case 0: { /* Child */
      int rc = parse_command(cmd2, level + 1, father);
      exit(rc);
    } default: { /* Parent */
      break;
    }
  }

  /* Wait for childs */
  int status1, status2;
  waitpid(pid1, &status1, 0);
  waitpid(pid2, &status2, 0);

  return status2;
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2)
 */
static bool do_on_pipe(command_t *cmd1, command_t *cmd2, int level,
    command_t *father) {
  /* Create pipe */
  int fd[2];
  if (pipe(fd) != 0) {
    fprintf(stderr, "error pipe\n");
    exit(EXIT_FAILURE);
  }

  /* First command */
  int pid1 = fork();
  switch(pid1) {
    case -1: { /* Fork error */
      perror("Could not fork");
      exit(EXIT_FAILURE);
    } case 0: { /* Child */
      close(fd[0]);         /* Close unused read end */
      close(STDOUT_FILENO); /* Close stdout */
      dup(fd[1]);           /* Set write end of pipe as stdout */

      int rc = parse_command(cmd1, level+1, father);
      exit(rc);
    } default: { /* Parent */
      break;
    }
  }

  int pid2 = fork();
  switch(pid2) {
    case -1: { /* Fork error */
      perror("Could not fork");
      exit(EXIT_FAILURE);
    } case 0: { /* Child */
      close(fd[1]);        /* Close unused write end */
      close(STDIN_FILENO); /* Close stdin */
      dup(fd[0]);          /* Set read end of pipe as stdin */

      int rc = parse_command(cmd2, level+1, father);
      exit(rc);
    } default: { /* Parent */
      break;
    }
  }

  close(fd[0]); /* Close unused read end */
  close(fd[1]); /* Close unused write end */

  /* Wait for childs */
  int status1, status2;
  waitpid(pid1, &status1, 0);
  waitpid(pid2, &status2, 0);

  return status2;
}

/**
 * Redirect input, output and error of s
 */
static void redirect_all(simple_command_t *s) {
  redirect_in(s);
  redirect_out(s);
  redirect_err(s);  
}

/**
 * Redirect input of s
 */
static void redirect_in(simple_command_t *s) {
  if (s->in != NULL) {
    char *filename = get_word(s->in);
    int in_fd = open(filename, O_RDONLY);
    free(filename);
    if (in_fd < 0) {
      perror("Could not open input file");
      exit(EXIT_FAILURE);
    }

    int rc = dup2(in_fd, STDIN_FILENO);
    if (rc < 0) {
      perror("Could not duplicate STDIN_FILENO");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Redirect output of s
 */
static void redirect_out(simple_command_t *s) {
  if (s->out != NULL) {
    int out_fd;
    char *filename = get_word(s->out);
    if (s->io_flags == IO_REGULAR) {
      out_fd = open(filename, O_WRONLY | O_TRUNC  | O_CREAT, IO_MODE);
      free(filename);
    } else if (s->io_flags == IO_OUT_APPEND) {
      out_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, IO_MODE);
      free(filename);
    } else {
      free(filename);
      mfatal("out io_flags = %d", s->io_flags);
    }

    if (out_fd < 0) {
      perror("Could not open output file");
      exit(EXIT_FAILURE);
    }

    int rc = dup2(out_fd, STDOUT_FILENO);
    if (rc < 0) {
      perror("Could not duplicate STDOUT_FILENO");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Redirect error of s
 */
static void redirect_err(simple_command_t *s) {
  if (s->err != NULL) {
    int err_fd;
    char *filename_err = get_word(s->err);
    if (s->io_flags == IO_REGULAR) {
      char *filename_out = get_word(s->out);
      if (s->out != NULL && strcmp(filename_err, filename_out) == 0) {
        err_fd = STDOUT_FILENO;
      } else {
        err_fd = open(filename_err, O_WRONLY | O_TRUNC | O_CREAT, IO_MODE);
      }
      free(filename_err);
      free(filename_out);
    } else if (s->io_flags == IO_ERR_APPEND) {
      err_fd = open(filename_err, O_WRONLY | O_APPEND | O_CREAT, IO_MODE);
      free(filename_err);
    } else {
      free(filename_err);
      mfatal("Unrecognized io_flags: %d", s->io_flags);
    }

    if (err_fd < 0) {
      perror("Could not open error file");
      exit(EXIT_FAILURE);
    }

    int rc = dup2(err_fd, STDERR_FILENO);
    if (rc < 0) {
      perror("Could not duplicate STDERR_FILENO");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Concatenate parts of the word to obtain the command
 */
static char *get_word(word_t *s) {
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
static char **get_argv(simple_command_t *command, int *size) {
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
