/***
*       ___           ___           ___           ___     
*      /\  \         /\  \         /\  \         /\__\    
*      \:\  \       /::\  \       /::\  \       /:/  /    
*       \:\  \     /:/\:\  \     /:/\ \  \     /:/__/     
*       /::\  \   /::\~\:\  \   _\:\~\ \  \   /::\  \ ___ 
*      /:/\:\__\ /:/\:\ \:\__\ /\ \:\ \ \__\ /:/\:\  /\__\
*     /:/  \/__/ \/__\:\/:/  / \:\ \:\ \/__/ \/__\:\/:/  /
*    /:/  /           \::/  /   \:\ \:\__\        \::/  / 
*    \/__/            /:/  /     \:\/:/  /        /:/  /  
*                    /:/  /       \::/  /        /:/  /   
*                    \/__/         \/__/         \/__/    
*
* TASH (TAangsongxiaoba SHell)
*
* Created on Fri Apr 18 2025
*
* Copyright (c) 2025 tangsongxiaoba 
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PROMPT "tash> "
#define BUF_INC_SIZE 128
#define DELIM " \t\r\n"
#define STATUS_OK 1
#define STATUS_END 0
#define NUM_BUILTINS (sizeof(builtin_str) / sizeof(char *))

#define GETLINE_FAILED ":( tash: get new line failed"
#define MEM_ALLOC_FAILED ":( tash: mem alloc failed"
#define EXEC_FAILED ":( tash: exec prog failed"
#define FORK_FAILED ":( tash: fork failed"
#define CD_NO_ARGS ":( tash: expected argument for \"cd\"\n"
#define CD_FAILED ":( tash: change dir failed"

int tash_cd(char **);
int tash_exit(char **);

char *tash_read();
char **tash_split(char *);
int tash_launch(char **);
int tash_exec(char **);
void tash_loop();

char *builtin_str[] = {
    "cd",
    "exit"
};

int (*builtin_func[])(char **) = {
    &tash_cd,
    &tash_exit
};

int main() {
    printf("-----------------------------------------------------------\n");
    printf("      ___           ___           ___           ___     \n");
    printf("     /\\  \\         /\\  \\         /\\  \\         /\\__\\    \n");
    printf("     \\:\\  \\       /::\\  \\       /::\\  \\       /:/  /    \n");
    printf("      \\:\\  \\     /:/\\:\\  \\     /:/\\ \\  \\     /:/__/     \n");
    printf("      /::\\  \\   /::\\~\\:\\  \\   _\\:\\~\\ \\  \\   /::\\  \\ ___ \n");
    printf("     /:/\\:\\__\\ /:/\\:\\ \\:\\__\\ /\\ \\:\\ \\ \\__\\ /:/\\:\\  /\\__\\\n");
    printf("    /:/  \\/__/ \\/__\\:\\/:/  / \\:\\ \\:\\ \\/__/ \\/__\\:\\/:/  /\n");
    printf("   /:/  /           \\::/  /   \\:\\ \\:\\__\\        \\::/  / \n");
    printf("   \\/__/            /:/  /     \\:\\/:/  /        /:/  /  \n");
    printf("                   /:/  /       \\::/  /        /:/  /   \n");
    printf("                   \\/__/         \\/__/         \\/__/    \n");
    printf("\n-----------------------------------------------------------\n\n");
    printf("Welcome to use TASH (TAngsongxiaoba SHell)!\n\n");
    tash_loop();
    return 0;
}

int tash_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, CD_NO_ARGS);
    } else {
        if (chdir(args[1]) != 0) {
            perror(CD_FAILED);
        }
    }
    return STATUS_OK;
}

int tash_exit(char **args) {
    return STATUS_END;
}

char *tash_read() {
    char *line = NULL;
    size_t buf_size = 0;
    if(getline(&line, &buf_size, stdin) == -1) {
        if (feof(stdin)) {
            printf("\n");
            exit(EXIT_SUCCESS);
        } else {
            perror(GETLINE_FAILED);
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **tash_split(char *line) {
    if (line == NULL) {
        return NULL;
    }

    size_t n = BUF_INC_SIZE;
    size_t pos = 0;

    char **tokens = (char **)malloc(n * sizeof(char *));
    if (tokens == NULL) {
        perror(MEM_ALLOC_FAILED);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, DELIM);
    while (token != NULL) {
        tokens[pos++] = token;
        if (pos >= n) {
            n += BUF_INC_SIZE;
            tokens = realloc(tokens, n * sizeof(char *));
            if (tokens == NULL) {
                perror(MEM_ALLOC_FAILED);
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, DELIM);
    }
    tokens[pos] = NULL;
    return tokens;
}

int tash_launch(char **args) {
    pid_t pid;
    int status;

    pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror(EXEC_FAILED);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror(FORK_FAILED);
    } else {
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int tash_exec(char **args) {
    if (args[0] == NULL) {
        return 1;
    }

    for (int i = 0; i < NUM_BUILTINS; ++i) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return tash_launch(args);
}

void tash_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("%s", PROMPT);
        line = tash_read();
        if (line && *line == '\0') {
            free(line);
            continue;
        }
        args = tash_split(line);
        status = tash_exec(args);

        free(line);
        free(args);
    } while(status);
}
