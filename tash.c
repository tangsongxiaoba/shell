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

#define PROMPT KGRN "tash> " KNRM
#define BUF_INC_SIZE 128
#define DELIM " \t\r\n"
#define STATUS_OK 1
#define STATUS_END 0

#define GETLINE_FAILED KRED ":( tash: get new line failed" KNRM
#define MEM_ALLOC_FAILED KRED ":( tash: mem alloc failed" KNRM
#define EXEC_FAILED KRED ":( tash: exec prog failed" KNRM
#define FORK_FAILED KRED ":( tash: fork failed" KNRM
#define SYNTAX_ERROR_NEWLINE_LT KRED ":( tash: syntax error near unexpected token `newline' for `<'" KNRM
#define SYNTAX_ERROR_NEWLINE_GT KRED ":( tash: syntax error near unexpected token `newline' for `>'" KNRM
#define SYNTAX_ERROR_NEWLINE_GTGT KRED ":( tash: syntax error near unexpected token `newline' for `>>'" KNRM
#define TOO_MANY_ARGS KRED ":( tash: too many arguments for a command segment" KNRM
#define OPEN_INPUT_FAILED KRED ":( tash: open input file" KNRM
#define DUP2_STDIN_FAILED KRED ":( tash: dup2 stdin failed" KNRM
#define OPEN_OUTPUT_FAILED KRED ":( tash: open output file" KNRM
#define DUP2_STDOUT_FAILED KRED ":( tash: dup2 stdout failed" KNRM
#define CD_MISSING_ARG KRED ":( tash: cd: missing argument" KNRM
#define CD_FAILED KRED ":( tash: cd" KNRM
#define SYNTAX_ERROR_PIPE KRED ":( tash: syntax error near `|'" KNRM
#define TOO_MANY_CMDS_PIPE KRED ":( tash: too many commands in pipeline" KNRM
#define PIPE_FAILED KRED ":( tash: pipe failed" KNRM
#define CHILD_DUP2_STDIN_FAILED KRED ":( tash: child dup2 stdin failed" KNRM
#define CHILD_DUP2_STDOUT_FAILED KRED ":( tash: child dup2 stdout failed" KNRM

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KBLD  "\x1B[1m"

char *tash_read();
char **tash_split(char *);
int tash_exec(char **);
void tash_loop();
void tash_prepare_exec(char **);

int main() {
    printf("%s-----------------------------------------------------------%s\n", KCYN, KNRM);
    printf("%s      ___           ___           ___           ___     %s\n", KBLU, KNRM);
    printf("%s     /\\  \\         /\\  \\         /\\  \\         /\\__\\    %s\n", KBLU, KNRM);
    printf("%s     \\:\\  \\       /::\\  \\       /::\\  \\       /:/  /    %s\n", KBLU, KNRM);
    printf("%s      \\:\\  \\     /:/\\:\\  \\     /:/\\ \\  \\     /:/__/     %s\n", KBLU, KNRM);
    printf("%s      /::\\  \\   /::\\~\\:\\  \\   _\\:\\~\\ \\  \\   /::\\  \\ ___ %s\n", KBLU, KNRM);
    printf("%s     /:/\\:\\__\\ /:/\\:\\ \\:\\__\\ /\\ \\:\\ \\ \\__\\ /:/\\:\\  /\\__\\%s\n", KBLU, KNRM);
    printf("%s    /:/  \\/__/ \\/__\\:\\/:/  / \\:\\ \\:\\ \\/__/ \\/__\\:\\/:/  /%s\n", KBLU, KNRM);
    printf("%s   /:/  /           \\::/  /   \\:\\ \\:\\__\\        \\::/  / %s\n", KBLU, KNRM);
    printf("%s   \\/__/            /:/  /     \\:\\/:/  /        /:/  /  %s\n", KBLU, KNRM);
    printf("%s                   /:/  /       \\::/  /        /:/  /   %s\n", KBLU, KNRM);
    printf("%s                   \\/__/         \\/__/         \\/__/    %s\n", KBLU, KNRM);
    printf("\n%s-----------------------------------------------------------%s\n\n", KCYN, KNRM);
    printf("%sWelcome to use TASH (TAngsongxiaoba SHell)!%s\n\n", KYEL KBLD, KNRM);
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
            fprintf(stderr, "%s: ", GETLINE_FAILED);
            perror(NULL);
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
        fprintf(stderr, "%s: ", MEM_ALLOC_FAILED);
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, DELIM);
    while (token != NULL) {
        tokens[pos++] = token;
        if (pos >= n) {
            n += BUF_INC_SIZE;
            tokens = realloc(tokens, n * sizeof(char *));
            if (tokens == NULL) {
                fprintf(stderr, "%s: ", MEM_ALLOC_FAILED);
                perror(NULL);
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
            fprintf(stderr, "%s: ", OPEN_INPUT_FAILED);
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            fprintf(stderr, "%s: ", DUP2_STDIN_FAILED);
            perror(NULL);
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
            fprintf(stderr, "%s: ", OPEN_OUTPUT_FAILED);
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            fprintf(stderr, "%s: ", DUP2_STDOUT_FAILED);
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }

    if (clean_argc == 0) {
        exit(EXIT_SUCCESS);
    }

    if (execvp(clean_args[0], clean_args) == -1) {
        fprintf(stderr, "%s: ", EXEC_FAILED);
        perror(NULL);
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
                fprintf(stderr, "%s: ", CD_FAILED);
                perror(NULL);
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
                fprintf(stderr, "%s: ", PIPE_FAILED);
                perror(NULL);
                if (in_fd != STDIN_FILENO) close(in_fd);
                for(int j = 0; j < i; j++) waitpid(pids[j], NULL, 0);
                return STATUS_OK; 
            }
        }

        pids[i] = fork();
        if (pids[i] == -1) {
            fprintf(stderr, "%s: ", FORK_FAILED);
            perror(NULL);
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
                    fprintf(stderr, "%s: ", CHILD_DUP2_STDIN_FAILED);
                    perror(NULL);
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }

            if (i < num_cmds - 1) {
                close(pipe_fds[0]);
                if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
                    fprintf(stderr, "%s: ", CHILD_DUP2_STDOUT_FAILED);
                    perror(NULL);
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