#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"

int main() {
    const char input[] = "echo file.txt | grep bruh";

    lexer_t lexer = {0}; 

    strncpy(lexer.input, input, MAX_INPUT_LENGTH);
    lexer.input_length = strlen(input);

    printf("Input: %s\n", lexer.input);
    printf("Input length (in bytes): %zu\n", lexer.input_length);

    run(&lexer);

    for (size_t i = 0; i < lexer.token_count; i++) {
        token_t token = lexer.tokens[i];
        
        printf("Token type: %d\n", token.type);
        printf("Token text: %s\n", token.text);
    }

    return 0;
}
