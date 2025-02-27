#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// Cria um prompt seguindo a formatação desejada ("[name:dir]$ ")
char *create_prompt(char *hostname, char *directory) {
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

int main() {
    // Buffers para informações do prompt
    char hostname[256];
    char directory[1024];

    // Atribui valores aos buffers
    gethostname(hostname, sizeof(hostname));
    getcwd(directory, sizeof(directory));

    // Cria prompt e buffer para o input
    char *prompt = create_prompt(hostname, directory);
    char *input;

    // Loop principal do shell
    while (1) {

        // Mostra o prompt e captura o input do usuário
        input = readline(prompt);
        
        // End of file, fim da execução do uspsh
        if (input == NULL)
            break;

        // Guarda no histórico
        add_history(input);

        free(input);
    }

    return 0;
}