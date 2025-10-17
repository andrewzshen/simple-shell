#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lexer.h"

#define PROMPT "shell: "

void execute_tokens(token_t *tokens, size_t token_count);

int main(int argc, char **argv) {
    if (argc > 2) {
        perror("Shell usage: ./main [-n]");
        return 1;
    }

    int show_prompt = 1;

    if (argc > 1 && (strcmp(argv[1], "-n") == 0)) {
        show_prompt = 0;
    }

    char input[MAX_INPUT_LENGTH];

    while (1) {
        if (show_prompt) {
            printf(PROMPT);
            fflush(stdout);
        }

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        lexer_t lexer = {0};

        strncpy(lexer.input, input, MAX_INPUT_LENGTH);
        lexer.input_length = strlen(input);

        run(&lexer);
        
        execute_tokens(lexer.tokens, lexer.token_count);
    }

    return 0;
}

void execute_command(char **argv, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("ERROR: fork failed");
        return;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        perror("ERROR: exec failed");
        exit(1);
    }

    if (!background) {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_pipeline(char ***commands, size_t cmd_count, int background) {
    int pipefd[2];
    int prev_fd = -1;

    for (size_t i = 0; i < cmd_count; i++) {
        if (i < cmd_count - 1) {
            if (pipe(pipefd) == -1) {
                perror("ERROR: pipe failed");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("ERROR: fork failed");
            return;
        }

        if (pid == 0) {
            if (prev_fd != -1) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < cmd_count - 1) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
            }
            execvp(commands[i][0], commands[i]);
            perror("ERROR: exec failed");
            _exit(1);
        }

        if (prev_fd != -1) {
            close(prev_fd);
        }

        if (i < cmd_count - 1) {
            close(pipefd[1]);
            prev_fd = pipefd[0];
        }
    }

    if (!background) {
        for (size_t i = 0; i < cmd_count; i++) {
            wait(NULL);
        }
    }
}

void execute_tokens(token_t *tokens, size_t token_count) {
    size_t i = 0;

    while (i < token_count) {
        char *argv[64];
        size_t argc = 0;

        char **pipeline_cmds[16];
        size_t pipeline_count = 0;

        int background = 0;

        while (i < token_count) {
            if (tokens[i].type == TOKEN_WORD) {
                argv[argc++] = tokens[i].lexeme;
                i++;
            } else if (tokens[i].type == TOKEN_PIPE) {
                argv[argc] = NULL;
                pipeline_cmds[pipeline_count] = malloc(sizeof(char*) * (argc + 1));
                for (size_t j = 0; j <= argc; j++) {
                    pipeline_cmds[pipeline_count][j] = argv[j];
                }
                pipeline_count++;
                argc = 0;
                i++;
            } else if (tokens[i].type == TOKEN_BACKGROUND) {
                background = 1;
                i++;
                break;
            } else if (tokens[i].type == TOKEN_EOF) {
                break;
            } else {
                i++;
            }
        }

        if (argc > 0) {
            argv[argc] = NULL;
            pipeline_cmds[pipeline_count] = malloc(sizeof(char*) * (argc + 1));

            for (size_t j = 0; j <= argc; j++) {
                pipeline_cmds[pipeline_count][j] = argv[j];
            }

            pipeline_count++;
        }

        if (pipeline_count == 1) {
            execute_command(pipeline_cmds[0], background);
        } else if (pipeline_count > 1) {
            execute_pipeline(pipeline_cmds, pipeline_count, background);
        }

        for (size_t k = 0; k < pipeline_count; k++) {
            free(pipeline_cmds[k]);
        }
    }
}
