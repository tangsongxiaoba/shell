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
#include <fcntl.h>

#define PROMPT "tash> "
#define BUF_INC_SIZE 128
#define DELIM " \t\r\n"
#define STATUS_OK 1
#define STATUS_END 0

#define GETLINE_FAILED ":( tash: get new line failed"
#define MEM_ALLOC_FAILED ":( tash: mem alloc failed"
#define EXEC_FAILED ":( tash: exec prog failed"
#define FORK_FAILED ":( tash: fork failed"
#define SYNTAX_ERROR_NEWLINE_LT ":( tash: syntax error near unexpected token `newline' for `<'"
#define SYNTAX_ERROR_NEWLINE_GT ":( tash: syntax error near unexpected token `newline' for `>'"
#define SYNTAX_ERROR_NEWLINE_GTGT ":( tash: syntax error near unexpected token `newline' for `>>'"
#define TOO_MANY_ARGS ":( tash: too many arguments for a command segment"
#define OPEN_INPUT_FAILED ":( tash: open input file"
#define DUP2_STDIN_FAILED ":( tash: dup2 stdin failed"
#define OPEN_OUTPUT_FAILED ":( tash: open output file"
#define DUP2_STDOUT_FAILED ":( tash: dup2 stdout failed"
#define CD_MISSING_ARG ":( tash: cd: missing argument"
#define CD_FAILED ":( tash: cd"
#define SYNTAX_ERROR_PIPE ":( tash: syntax error near `|'"
#define TOO_MANY_CMDS_PIPE ":( tash: too many commands in pipeline"
#define PIPE_FAILED ":( tash: pipe failed"
#define CHILD_DUP2_STDIN_FAILED ":( tash: child dup2 stdin failed"
#define CHILD_DUP2_STDOUT_FAILED ":( tash: child dup2 stdout failed"

char *tash_read();
char **tash_split(char *);
int tash_exec(char **);
void tash_loop();
void tash_prepare_exec(char **);

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

void tash_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("%s", PROMPT);
        line = tash_read();
        args = tash_split(line);
        status = tash_exec(args);

        free(line);
        free(args);
    } while(status);
}

void tash_prepare_exec(char **args) {
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;
    
    char *clean_args[BUF_INC_SIZE]; 
    int clean_argc = 0;
    int i = 0;

    while(args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1] != NULL) {
                input_file = args[i+1];
                i++;
            } else {
                fprintf(stderr, "%s\n", SYNTAX_ERROR_NEWLINE_LT);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i+1] != NULL) {
                output_file = args[i+1];
                append_output = 0;
                i++; 
            } else {
                fprintf(stderr, "%s\n", SYNTAX_ERROR_NEWLINE_GT);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (args[i+1] != NULL) {
                output_file = args[i+1];
                append_output = 1;
                i++; 
            } else {
                fprintf(stderr, "%s\n", SYNTAX_ERROR_NEWLINE_GTGT);
                exit(EXIT_FAILURE);
            }
        } else {
            if (clean_argc < BUF_INC_SIZE - 1) {
                 clean_args[clean_argc++] = args[i];
            } else {
                fprintf(stderr, "%s\n", TOO_MANY_ARGS);
                exit(EXIT_FAILURE);
            }
        }
        i++;
    }
    clean_args[clean_argc] = NULL;

    if (input_file) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in == -1) {
            perror(OPEN_INPUT_FAILED);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            perror(DUP2_STDIN_FAILED);
            exit(EXIT_FAILURE);
        }
        close(fd_in);
    }

    if (output_file) {
        int flags = O_WRONLY | O_CREAT;
        if (append_output) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }
        int fd_out = open(output_file, flags, 0644);
        if (fd_out == -1) {
            perror(OPEN_OUTPUT_FAILED);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            perror(DUP2_STDOUT_FAILED);
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }

    if (clean_argc == 0) {
        exit(EXIT_SUCCESS);
    }

    if (execvp(clean_args[0], clean_args) == -1) {
        perror(EXEC_FAILED);
        exit(EXIT_FAILURE); 
    }
}

int tash_exec(char **args) {
    if (args[0] == NULL) {
        return STATUS_OK;
    }

    if (strcmp(args[0], "exit") == 0) {
        return STATUS_END;
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "%s\n", CD_MISSING_ARG);
        } else {
            if (chdir(args[1]) != 0) {
                perror(CD_FAILED);
            }
        }
        return STATUS_OK;
    }

    char **cmds[BUF_INC_SIZE / 2];
    int num_cmds = 0;
    
    cmds[num_cmds++] = args;
    for (int k = 0; args[k] != NULL; k++) {
        if (strcmp(args[k], "|") == 0) {
            args[k] = NULL;
            if (args[k+1] == NULL || strcmp(args[k+1], "|") == 0 ) {
                fprintf(stderr, "%s\n", SYNTAX_ERROR_PIPE);
                return STATUS_OK;
            }
            if (num_cmds < (BUF_INC_SIZE / 2)) {
                cmds[num_cmds++] = &args[k+1];
            } else {
                fprintf(stderr, "%s\n", TOO_MANY_CMDS_PIPE);
                return STATUS_OK;
            }
        }
    }
    pid_t pids[num_cmds];
    int pipe_fds[2];
    int in_fd = STDIN_FILENO;
    for (int i = 0; i < num_cmds; i++) {
        char **current_cmd_args = cmds[i];

        if (i < num_cmds - 1) {
            if (pipe(pipe_fds) == -1) {
                perror(PIPE_FAILED);
                if (in_fd != STDIN_FILENO) close(in_fd);
                for(int j = 0; j < i; j++) waitpid(pids[j], NULL, 0);
                return STATUS_OK; 
            }
        }

        pids[i] = fork();
        if (pids[i] == -1) {
            perror(FORK_FAILED);
            if (in_fd != STDIN_FILENO) close(in_fd);
            if (i < num_cmds - 1) {
                close(pipe_fds[0]); 
                close(pipe_fds[1]); 
            }
            for(int j = 0; j < i; j++) waitpid(pids[j], NULL, 0);
            return STATUS_OK;
        }

        if (pids[i] == 0) {
            if (in_fd != STDIN_FILENO) {
                if (dup2(in_fd, STDIN_FILENO) == -1) {
                    perror(CHILD_DUP2_STDIN_FAILED); 
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }

            if (i < num_cmds - 1) {
                close(pipe_fds[0]);
                if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
                    perror(CHILD_DUP2_STDOUT_FAILED);
                    exit(EXIT_FAILURE);
                }
                close(pipe_fds[1]);
            }
            tash_prepare_exec(current_cmd_args);
        }

        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (i < num_cmds - 1) {
            close(pipe_fds[1]);
            in_fd = pipe_fds[0];
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        int child_status; 
        waitpid(pids[i], &child_status, 0);
    }

    return STATUS_OK;
}