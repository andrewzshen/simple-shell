#include <stddef.h>

#ifndef LEXER_H
#define LEXER_H

#define MAX_TOKEN_LENGTH 32
#define MAX_INPUT_LENGTH 512

// I found this talk on lexical scanning in Go and it looked really cool and smart and interesting so I'm doing it here
// https://www.youtube.com/watch?v=HxaD_trXwRE

// This is probably a lot more useful for more complicated languages but I am writing a simple shell for CS 170 and a parser and lexer for CS 160
// so this has a lot more value

typedef enum token_type {
    TOKEN_ERROR,        // Sequence of illegal characters
    TOKEN_WORD,         // Things like cat, file.txt, etc.
    TOKEN_REDIRECT_IN,  // redirect in
    TOKEN_REDIRECT_OUT, // redirect out
    TOKEN_PIPE,         // pipe into next command
    TOKEN_BACKGROUND,   // run in background
    TOKEN_EOF           // \0
} token_type_t;

typedef struct token { 
    token_type_t type;   // What type of token it is
    char lexeme[MAX_TOKEN_LENGTH]; // The contents of the token
} token_t;

typedef struct lexer {
    char input[MAX_INPUT_LENGTH];
    size_t input_length;

    size_t start_pos; // The start index of the character the lexer is currently at
    size_t curr_pos; // The current index of the character the lexer is currently at

    token_t tokens[MAX_INPUT_LENGTH]; // The maximum number of tokens is just if every character is a token
    size_t token_count;
} lexer_t;

// Incredibly cursed struct function pointer hack
// Unfortunately C does not allow you to typedef a function pointer to itself,
// so you predefine a struct that contains nothing but a function pointer, and that function pointer can return the struct
typedef struct state state_t;
struct state {
    state_t *(*statefn)(lexer_t *lexer);
};

#define IS_METACHAR(ch) (ch == '>') || (ch == '<') || (ch == '|') || (ch == '&')

void run(lexer_t *lexer);

#endif
