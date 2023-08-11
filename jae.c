/* ---------------------------------------------------------------------------
 * author:   Austin Larsen
 * synopsis: jae (just an editor)
 * date:     Sun Jan 15 07:58:36 PM EST 2023 
 * ---------------------------------------------------------------------------
 */
#define _GNU_SOURCE
#include <stdio.h>

#include "term.h"
#include <stdbool.h>
#include <ctype.h>

int main( int argc, char **argv)
{
    Term term;

    if (argc > 1) {
        term_init(&term, argv[1]);
    } else {
        term_init(&term, NULL);
    }

    term_run(&term);
    return 0;
}
