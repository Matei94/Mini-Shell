/******************************************************************************
 * Data types and error logs
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: April 2015
 *****************************************************************************/

#ifndef _MINTERNALS_H
#define _MINTERNALS_H

#define LOG_FILE stdout
#define ERR_FILE stderr

#ifdef LOG
  #define mlog(msg, ...) fprintf(LOG_FILE, "[mlog in %s:%d] " msg "\n", \
    __FILE__, __LINE__, ##__VA_ARGS__)
#else
  #define mlog(msg, ...) /* Do nothing */
#endif

#ifdef ERR
  #define merr(msg, ...) fprintf(ERR_FILE, "[merr in %s:%d] " msg "\n", \
    __FILE__, __LINE__, ##__VA_ARGS__)
#else
  #define merr(msg, ...) /* Do nothing */
#endif

#define mfatal(msg, ...) do {                      \
  fprintf(ERR_FILE, "[mfatal in %s:%d] " msg "\n", \
    __FILE__, __LINE__, ##__VA_ARGS__);            \
  exit(EXIT_FAILURE);                              \
} while (0)

#endif /* _INTERNALS_H */
