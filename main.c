#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lexer.h"

#define PROMPT "shell: " 

void child_tokens(token_t *tokens, int start, int end, int in_fd, int out_fd);
void execute_tokens(token_t *tokens, size_t start, size_t end, int in_fd, int out_fd);

int build_args_from_tokens(token_t *tokens, int start, int end, char *argv[], int max_argv) {
    int argc = 0;
    for (int i = start; i <= end && argc < max_argv - 1; i++) {
        if (tokens[i].type == TOKEN_REDIRECT_IN || tokens[i].type == TOKEN_REDIRECT_OUT) {
            if (i + 1 <= end && tokens[i + 1].type == TOKEN_WORD) {
                i++;
            }
            continue;
        }

        if (tokens[i].type == TOKEN_WORD) {
            argv[argc++] = tokens[i].lexeme;
        }
    }

    argv[argc] = NULL;
    return argc;
}

int main(int argc, char *argv[]) {
    int no_prompt = (argc > 1 && strcmp(argv[1], "-n") == 0);
    char line[MAX_INPUT_LENGTH];

    while (1) {
        if (!no_prompt) {
            printf(PROMPT);
            fflush(stdout);
        }

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        lexer_t lexer = {0};

        strncpy(lexer.input, line, sizeof(lexer.input) - 1);
        lexer.input_length = strlen(lexer.input);
        
        run(&lexer);
    
        int background = 0;

        if (lexer.token_count >= 2 && lexer.tokens[lexer.token_count - 2].type == TOKEN_BACKGROUND) {
            background = 1;
            lexer.token_count--;
        }

        if (lexer.token_count > 0) {
            execute_tokens(lexer.tokens, 0, lexer.token_count - 1, STDIN_FILENO, STDOUT_FILENO);
        }

        if (!background) {
            while (waitpid(-1, NULL, 0) > 0);
        }
    }

    return 0;
}

void child_tokens(token_t *tokens, int start, int end, int in_fd, int out_fd) { 
    char *argv[MAX_INPUT_LENGTH];
    int argc = build_args_from_tokens(tokens, start, end, argv, MAX_INPUT_LENGTH);

    pid_t pid = fork();

    if (pid == 0) { 
        if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }

        if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        char *argv[MAX_INPUT_LENGTH];
        int argc = 0;

        for (int i = start; i <= end; i++) {
            if (tokens[i].type == TOKEN_REDIRECT_IN || tokens[i].type == TOKEN_REDIRECT_OUT) {
                if (i + 1 <= end && tokens[i + 1].type == TOKEN_WORD) {
                    i++;
                }
                continue;
            }

            if (tokens[i].type == TOKEN_WORD) {
                argv[argc++] = tokens[i].lexeme;
            }
        }

        argv[argc] = NULL;

        if (argc > 0) {
            execvp(argv[0], argv);
            fprintf(stderr, "ERROR: command not found: %s\n", argv[0]);
        }

        execvp(argv[0], argv);
        fprintf(stderr, "ERROR: command not found: %s\n", argv[0]);
        _exit(1);
    }

    if (pid < 0) {
        fprintf(stderr, "ERROR: fork failed\n");
        exit(1);
    }

    if (in_fd != STDIN_FILENO) {
        close(in_fd);
    }
    
    if (out_fd != STDOUT_FILENO) {
        close(out_fd);
    }
}

void execute_tokens(token_t *tokens, size_t start, size_t end, int in_fd, int out_fd) {
    int pipe_index = -1;

    for (size_t i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_PIPE) {
            pipe_index = i;
            break;
        }
    }
    
    if (pipe_index != -1) {
        if (pipe_index + 1 > end || tokens[pipe_index + 1].type == TOKEN_PIPE || tokens[pipe_index + 1].type == TOKEN_EOF) {
            fprintf(stderr, "ERROR: missing command after |\n");
            return;
        }
    }

    int real_in = in_fd;
    int real_out = out_fd;
    int cmd_start = start;
    int cmd_end = (pipe_index == -1) ? end : pipe_index - 1;

    for (size_t i = start; i <= cmd_end; i++) {
        if (tokens[i].type == TOKEN_REDIRECT_IN) {
            if (i + 1 > end || tokens[i + 1].type != TOKEN_WORD) {
                fprintf(stderr, "ERROR: missing input file after <\n");
                return;
            }

            real_in = open(tokens[i + 1].lexeme, O_RDONLY);
            
            if (real_in < 0) {
                fprintf(stderr, "ERROR: open input\n");
                return;
            }
            i++;
        } else if (tokens[i].type == TOKEN_REDIRECT_OUT) {
            if (i + 1 > end || tokens[i + 1].type != TOKEN_WORD) {
                fprintf(stderr, "ERROR: missing output file after >\n");
                return;
            }
            
            real_out = open(tokens[i + 1].lexeme, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            
            if (real_out < 0) {
                fprintf(stderr, "ERROR: open output\n");
                return;
            }
            i++;
        }
    }

    if (pipe_index == -1) {
        child_tokens(tokens, cmd_start, cmd_end, real_in, real_out);
        return;
    }

    int pipe_fd[2];
    
    if (pipe(pipe_fd) < 0) {
        fprintf(stderr, "ERROR: pipe\n");
        return;
    }

    child_tokens(tokens, cmd_start, cmd_end, real_in, pipe_fd[1]);
    close(pipe_fd[1]);

    execute_tokens(tokens, pipe_index + 1, end, pipe_fd[0], real_out);
    close(pipe_fd[0]);
}
