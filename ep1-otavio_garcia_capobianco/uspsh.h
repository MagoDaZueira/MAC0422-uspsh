#ifndef USPSH_H
#define USPSH_H

#include <stddef.h>

/******************************************************************
********************** GERENCIAMENTO DE INPUT ********************/

char *create_prompt(char *hostname, size_t hsize, char *directory, size_t dsize);
int count_args(char *str);
char **separate_args(char *input);

/******************************************************************
************************ COMANDOS INTERNOS ***********************/

void cd(char **args);
void whoami();
void ch_mod(char **args);

/******************************************************************
************************ PROGRAMAS EXTERNOS **********************/

void do_execve(char **args);
void execute_args(char **args);

#endif // USPSH_H
