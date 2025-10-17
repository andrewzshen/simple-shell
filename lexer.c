#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "lexer.h"

// Function declarations

static void emit(lexer_t *lexer, token_type_t type);

static char next(lexer_t *lexer);
static char peek(lexer_t *lexer);

static void ignore(lexer_t *lexer);
static void backup(lexer_t *lexer);

static bool accept(lexer_t *lexer, const char valid_chars[]);
static void accept_run(lexer_t *lexer, const char valid_chars[]);

static state_t *lex_base(lexer_t *lexer);
static state_t *lex_word(lexer_t *lexer);
static state_t *lex_metachar(lexer_t *lexer);

static state_t base_state = { .statefn = lex_base };
static state_t word_state = { .statefn = lex_word };
static state_t metachar_state = { .statefn = lex_metachar };

void run(lexer_t *lexer);

// Function definitions

static void emit(lexer_t *lexer, token_type_t type) {  
    token_t token;
    token.type = type;

    const char *lexeme_start = lexer->input + lexer->start_pos;
    const size_t lexeme_width = lexer->curr_pos - lexer->start_pos;
    strncpy(token.lexeme, lexeme_start, lexeme_width);
    token.lexeme[lexeme_width] = '\0';

    lexer->tokens[lexer->token_count] = token;
    lexer->token_count++;

    lexer->start_pos = lexer->curr_pos;
}

static char next(lexer_t *lexer) {
    if (lexer->curr_pos >= lexer->input_length) {
        return '\0';
    }

    return lexer->input[lexer->curr_pos++];
}

static char peek(lexer_t *lexer) { 
    char ch = next(lexer);
    backup(lexer);
    return ch;
}

static void ignore(lexer_t *lexer) {
    lexer->start_pos = lexer->curr_pos;
}

static void backup(lexer_t *lexer) {
    lexer->curr_pos--;
}

static bool accept(lexer_t *lexer, const char valid_chars[]) {
    if (strchr(valid_chars, next(lexer)) != NULL) {
        return true;
    }

    backup(lexer);
    return false;
}

static void accept_run(lexer_t *lexer, const char valid_chars[]) {
    while (strchr(valid_chars, next(lexer)) != NULL);
    backup(lexer);
}

static state_t *lex_base(lexer_t *lexer) {
    while (true) {
        char ch = next(lexer);

        if (ch == '\0') {
            break;
        } else if (isspace(ch)) {
            ignore(lexer);
        } else if (IS_METACHAR(ch)) {
            backup(lexer);
            return &metachar_state;
        } else {
            backup(lexer);
            return &word_state;
        }
    }

    emit(lexer, TOKEN_EOF);
    return NULL;
}

static state_t *lex_word(lexer_t *lexer) { 
    while (true) {
        char ch = next(lexer);

        if (ch == '\0') {
            break;
        } else if (isspace(ch) || IS_METACHAR(ch)) {
            backup(lexer);
            break;
        }
    }

    emit(lexer, TOKEN_WORD);
    return &base_state;
}

static state_t *lex_metachar(lexer_t *lexer) {
    char ch = next(lexer);

    switch (ch) {
        case '<': 
            emit(lexer, TOKEN_REDIRECT_IN); 
            break;
        case '>': 
            emit(lexer, TOKEN_REDIRECT_OUT); 
            break;
        case '|': 
            emit(lexer, TOKEN_PIPE); 
            break;
        case '&': 
            emit(lexer, TOKEN_BACKGROUND); 
            break;
        default: 
            emit(lexer, TOKEN_ERROR);
            break;
    }

    return &base_state;
}

void run(lexer_t *lexer) {
    state_t *state = &base_state;

    while (state != NULL) {
        state = state->statefn(lexer);
    }
}
