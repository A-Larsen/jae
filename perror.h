#ifndef _PERROR_H_
#define _PERROR_H_
#include <stdio.h>
#include <errno.h>

#define PERROR(s) fprintf(stderr, "\n\e[31mERROR:\e[39m\n\t%s\n\n" \
                                  "\e[93m(errno %d)\e[39m" \
                                  " -> \e[32m%s %s:%d\e[39m", s, errno, \
                                  __FILE__, __FUNCTION__, __LINE__)

#endif // _PERROR_H_
