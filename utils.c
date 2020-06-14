#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void check_err(int exp, char *msg)
{
    if (exp == -1)
    {
        printf("Error: %s\nError msg: %s", strerror(errno), msg);
        exit(EXIT_FAILURE);
    }
}