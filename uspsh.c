#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// Cria um prompt seguindo a formatação desejada ("[name:dir]$ ")
char *create_prompt() {
    // Buffers para informações do prompt
    char hostname[256];
    char directory[1024];

    // Atribui valores aos buffers
    gethostname(hostname, sizeof(hostname));
    getcwd(directory, sizeof(directory));

    // Tamanho necessário para o prompt
    size_t size = strlen(hostname) + strlen(directory) + strlen("[]:$ ");
    char *prompt = malloc(size);

    strcpy(prompt, "[");
    strcat(prompt, hostname);
    strcat(prompt, ":");
    strcat(prompt, directory);
    strcat(prompt, "]$ ");

    return prompt;
}

int count_args(char *str) {
    char copy[strlen(str) + 1];
    char *token;
    int count = 0;

    strcpy(copy, str);
    token = strtok(copy, " ");
    while (token != NULL) {
        count++;
        token = strtok(NULL, " ");
    }

    return count;
}


char **separate_args(char *input) {
    char **args;
    char *token;
    int count;
    
    count = count_args(input);
    args = malloc((count + 1) * sizeof(char *));
    
    int i = 0;
    token = strtok(input, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    return args;
}

void process_args(char **args) {
    char *command = args[0];

    if (!strcmp(command, "cd")) {
        chdir(args[1]);
    }
}

int main() {
    // Cria buffers para o prompt e para o input
    char *prompt;
    char *input;

    // Loop principal do shell
    while (1) {
        // Mostra o prompt e captura o input do usuário
        prompt = create_prompt();
        input = readline(prompt);
        
        // End of file, fim da execução do uspsh
        if (input == NULL)
            break;

        // Guarda no histórico
        add_history(input);

        char **args = separate_args(input);
        process_args(args);

        free(args);
        free(input);
    }

    return 0;
}
