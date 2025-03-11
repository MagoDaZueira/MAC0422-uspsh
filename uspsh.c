#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


// Cria um prompt seguindo a formatação desejada ("[name:dir]$ ")
char *create_prompt(char *hostname, size_t hsize, char *directory, size_t dsize) {
    // Atribui valores aos buffers
    gethostname(hostname, hsize);
    getcwd(directory, dsize);

    // Tamanho necessário para o prompt
    size_t size = strlen(hostname) + strlen(directory) + strlen("[]:$ ") + 1;
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

void cd(char **args) {
    if (args[1] != NULL)
        chdir(args[1]);
    else
        fprintf(stderr, "Uso correto: cd <diretorio>\n");
}

void whoami() {
    uid_t id = geteuid();
    struct passwd *pw = getpwuid(id);

    printf("%s\n", pw->pw_name);
}

void ch_mod(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Uso correto: chmod <permissao> <arquivo|diretorio>\n");
        return;
    }

    char *endptr;
    mode_t permission = strtol(args[1], &endptr, 8);
    char *path = args[2];

    chmod(path, permission);
}

int is_programa_bin(char *command) {
    return !strncmp(command, "/bin", 4) ||
           !strncmp(command, "/usr", 4);
}

void do_execve(char **args) {
    pid_t pid = fork(); // Novo processo

    // Processo filho
    if (pid == 0) {
        execve(args[0], args, NULL);
        exit(1); // Se chegou aqui é um erro
    }
    // Processo pai (espera filho terminar)
    else {
        waitpid(pid, NULL, 0);
    }
}

void execute_args(char **args) {
    char *command = args[0];

    if (!strcmp(command, "cd")) {
        cd(args);
    }
    else if (!strcmp(command, "whoami")) {
        whoami();
    }
    else if (!strcmp(command, "chmod")) {
        ch_mod(args);
    }
    else if (is_programa_bin(command)) {
        do_execve(args);
    }
    else if (!strcmp(command, "bye")) {
        exit(0);
    }
}

int main() {
    // Cria buffers para o prompt e para o input
    char hostname[256];
    char directory[1024];
    char *prompt;
    char *input;

    // Loop principal do shell
    while (1) {
        // Mostra o prompt e captura o input do usuário
        prompt = create_prompt(hostname, sizeof(hostname), directory, sizeof(directory));
        // prompt = create_prompt(hostname, directory);
        input = readline(prompt);
        
        // End of file, fim da execução do uspsh
        if (input == NULL)
            break;

        // Guarda no histórico
        add_history(input);

        // Trata o input e executa o comando correspondente
        char **args = separate_args(input);
        execute_args(args);

        // Libera memória
        free(args);
        free(input);
        free(prompt);
    }

    return 0;
}
