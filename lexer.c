#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexer.h"

static const char *keywords[MAX_KEYWORDS] =
{
    "int", "float", "return", "if", "else", "while",
    "for", "do", "break", "continue", "char", "double",
    "void", "switch", "case", "default", "const", "static",
    "sizeof", "struct"
};

static const char *operators[] =
{
    "++", "--", "->", "&&", "||", "==", "!=", "<=", ">=",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>",
    "+", "-", "*", "/", "%", "=", "!", "<", ">", "|", "&", "^", "~",
    NULL
};

static const char *specialCharacters = ",;{}()[]:.?";

static FILE *sourceFile = NULL;
static int pushedChar = -2;

static int readChar(void)
{
    int ch;

    if (pushedChar != -2)
    {
        ch = pushedChar;
        pushedChar = -2;
        return ch;
    }

    return fgetc(sourceFile);
}

static void unreadChar(int ch)
{
    if (ch != EOF)
        pushedChar = ch;
}

static void appendChar(char *buffer, int ch)
{
    size_t length = strlen(buffer);

    if (length < MAX_TOKEN_SIZE - 1)
    {
        buffer[length] = (char)ch;
        buffer[length + 1] = '\0';
    }
}

static Token makeToken(const char *lexeme)
{
    Token token;

    strncpy(token.lexeme, lexeme, MAX_TOKEN_SIZE - 1);
    token.lexeme[MAX_TOKEN_SIZE - 1] = '\0';

    categorizeToken(&token);

    return token;
}

static Token endToken(void)
{
    Token token;

    token.lexeme[0] = '\0';
    token.type = UNKNOWN;

    return token;
}

void initializeLexer(const char *filename)
{
    if (sourceFile != NULL)
    {
        fclose(sourceFile);
        sourceFile = NULL;
    }

    pushedChar = -2;
    sourceFile = fopen(filename, "r");

    if (sourceFile == NULL)
        perror("Error opening input file");
}

int isKeyword(const char *str)
{
    int i;

    for (i = 0; i < MAX_KEYWORDS; i++)
    {
        if (strcmp(str, keywords[i]) == 0)
            return 1;
    }

    return 0;
}

int isOperator(const char *str)
{
    int i;

    for (i = 0; operators[i] != NULL; i++)
    {
        if (strcmp(str, operators[i]) == 0)
            return 1;
    }

    return 0;
}

int isSpecialCharacter(char ch)
{
    return strchr(specialCharacters, ch) != NULL;
}

int isIdentifier(const char *str)
{
    int i;

    if (str == NULL || str[0] == '\0')
        return 0;

    if (!(isalpha((unsigned char)str[0]) || str[0] == '_'))
        return 0;

    for (i = 1; str[i] != '\0'; i++)
    {
        if (!(isalnum((unsigned char)str[i]) || str[i] == '_'))
            return 0;
    }

    return !isKeyword(str);
}

int isConstant(const char *str)
{
    char *end;
    size_t length;

    if (str == NULL || str[0] == '\0')
        return 0;

    length = strlen(str);

    if (length >= 3 && str[0] == '\'' && str[length - 1] == '\'')
        return 1;

    if (length >= 2 && str[0] == '"' && str[length - 1] == '"')
        return 1;

    (void)strtod(str, &end);

    if (end != str && *end == '\0')
        return 1;

    if (isdigit((unsigned char)str[0]))
    {
        char copy[MAX_TOKEN_SIZE];
        size_t n = length;

        if (n >= sizeof(copy))
            return 0;

        strcpy(copy, str);

        while (n > 0 &&
               (copy[n - 1] == 'u' || copy[n - 1] == 'U' ||
                copy[n - 1] == 'l' || copy[n - 1] == 'L'))
        {
            copy[--n] = '\0';
        }

        if (n > 0)
        {
            (void)strtod(copy, &end);

            if (end != copy && *end == '\0')
                return 1;
        }
    }

    return 0;
}

void categorizeToken(Token *token)
{
    if (token == NULL)
        return;

    if (isKeyword(token->lexeme))
        token->type = KEYWORD;
    else if (isOperator(token->lexeme))
        token->type = OPERATOR;
    else if (token->lexeme[0] != '\0' &&
             token->lexeme[1] == '\0' &&
             isSpecialCharacter(token->lexeme[0]))
        token->type = SPECIAL_CHARACTER;
    else if (isConstant(token->lexeme))
        token->type = CONSTANT;
    else if (isIdentifier(token->lexeme))
        token->type = IDENTIFIER;
    else
        token->type = UNKNOWN;
}

static Token readQuotedToken(int quote)
{
    char buffer[MAX_TOKEN_SIZE] = "";
    int ch;

    appendChar(buffer, quote);

    while ((ch = readChar()) != EOF)
    {
        appendChar(buffer, ch);

        if (ch == '\\')
        {
            int escaped = readChar();

            if (escaped == EOF)
                break;

            appendChar(buffer, escaped);
        }
        else if (ch == quote)
        {
            break;
        }
        else if (ch == '\n')
        {
            break;
        }
    }

    return makeToken(buffer);
}

static Token readWordToken(int first)
{
    char buffer[MAX_TOKEN_SIZE] = "";
    int ch;

    appendChar(buffer, first);

    while ((ch = readChar()) != EOF &&
           (isalnum((unsigned char)ch) || ch == '_'))
    {
        appendChar(buffer, ch);
    }

    unreadChar(ch);

    return makeToken(buffer);
}

static Token readNumberToken(int first, int startsWithDot)
{
    char buffer[MAX_TOKEN_SIZE] = "";
    int ch;

    if (startsWithDot)
        appendChar(buffer, '.');

    appendChar(buffer, first);

    while ((ch = readChar()) != EOF)
    {
        if (isalnum((unsigned char)ch) || ch == '.' || ch == '_')
        {
            appendChar(buffer, ch);
        }
        else if ((ch == '+' || ch == '-') &&
                 strlen(buffer) > 0 &&
                 (buffer[strlen(buffer) - 1] == 'e' ||
                  buffer[strlen(buffer) - 1] == 'E' ||
                  buffer[strlen(buffer) - 1] == 'p' ||
                  buffer[strlen(buffer) - 1] == 'P'))
        {
            appendChar(buffer, ch);
        }
        else
        {
            unreadChar(ch);
            break;
        }
    }

    return makeToken(buffer);
}

static Token readOperatorToken(int first)
{
    char buffer[MAX_TOKEN_SIZE] = "";
    char best[MAX_TOKEN_SIZE] = "";
    int ch;
    int second;

    appendChar(buffer, first);

    second = readChar();

    if (second != EOF)
    {
        appendChar(buffer, second);

        if (isOperator(buffer))
            strcpy(best, buffer);

        if (strcmp(buffer, "<<") == 0 || strcmp(buffer, ">>") == 0)
        {
            ch = readChar();

            if (ch == '=')
            {
                appendChar(buffer, ch);
                strcpy(best, buffer);
            }
            else
            {
                unreadChar(ch);
            }
        }
        else if (!isOperator(buffer))
        {
            unreadChar(second);
            buffer[1] = '\0';
        }
    }

    if (best[0] != '\0')
        return makeToken(best);

    if (isOperator(buffer))
        return makeToken(buffer);

    buffer[1] = '\0';

    return makeToken(buffer);
}

Token getNextToken(void)
{
    int ch;

    if (sourceFile == NULL)
        return endToken();

    while ((ch = readChar()) != EOF)
    {
        char buffer[MAX_TOKEN_SIZE] = "";

        if (isspace((unsigned char)ch))
            continue;

        if (ch == '#')
        {
            while ((ch = readChar()) != EOF && ch != '\n')
                ;

            continue;
        }

        if (ch == '/')
        {
            int next = readChar();

            if (next == '/')
            {
                while ((ch = readChar()) != EOF && ch != '\n')
                    ;

                continue;
            }

            if (next == '*')
            {
                int previous = 0;

                while ((ch = readChar()) != EOF)
                {
                    if (previous == '*' && ch == '/')
                        break;

                    previous = ch;
                }

                continue;
            }

            unreadChar(next);

            return makeToken("/");
        }

        if (ch == '\'' || ch == '"')
            return readQuotedToken(ch);

        if (isalpha((unsigned char)ch) || ch == '_')
            return readWordToken(ch);

        if (isdigit((unsigned char)ch))
            return readNumberToken(ch, 0);

        if (ch == '.')
        {
            int next = readChar();

            if (isdigit((unsigned char)next))
                return readNumberToken(next, 1);

            unreadChar(next);

            buffer[0] = '.';
            buffer[1] = '\0';

            return makeToken(buffer);
        }

        if (strchr("+-*/%=!<>&|^~", ch) != NULL)
            return readOperatorToken(ch);

        if (isSpecialCharacter((char)ch))
        {
            buffer[0] = (char)ch;
            buffer[1] = '\0';

            return makeToken(buffer);
        }

        buffer[0] = (char)ch;
        buffer[1] = '\0';

        return makeToken(buffer);
    }

    return endToken();
}