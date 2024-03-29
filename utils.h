/******************************************************************************
 * Mini Shell in Linux - API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: April 2015
 *****************************************************************************/

#ifndef _UTILS_H
#define _UTILS_H

#include "parser.h"

#define CHUNK_SIZE 100
#define ERR_ALLOCATION "unable to allocate memory"

#define SHELL_EXIT -100

#define IO_MODE 0664

/**
 * Readline from mini-shell.
 */
char *read_line();

/**
 * Parse and execute a command.
 */
int parse_command(command_t *, int, command_t *);

#endif
