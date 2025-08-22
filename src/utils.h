#ifndef UTILS_H
#define UTILS_H

/* Return the duplicate of the value of envariable NAME, or NULL if it doesn't exist.  */
extern char *get_env(const char *__name) __nonnull ((1)) __wur;

/* Test for access to NAME using the real UID and real GID.  */
extern int access_file (const char *path, int amode) __nonnull ((1));

#endif