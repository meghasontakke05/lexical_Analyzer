#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[])
{
    Token token;

    if (argc != 2)
    {
        printf("Usage: %s <filename.c>\n", argv[0]);
        return 1;
    }

    initializeLexer(argv[1]);

    while (1)
    {
        token = getNextToken();

        if (token.lexeme[0] == '\0')
            break;

        printf("Token: %s, Type: %d\n",
               token.lexeme, token.type);
    }

    return 0;
}