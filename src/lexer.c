
#include <ctype.h>
#include <stddef.h>
#include "lexer.h"

static char lexer_peek(const Lexer *lexer)
{
    return lexer->source[lexer->current];
}

static char lexer_advance(Lexer *lexer)
{
    char c = lexer->source[lexer->current];

    if (c != '\0')
    {
        lexer->current++;

        if (c == '\n')
        {
            lexer->line++;
            lexer->column = 1;
        }
        else
        {
            lexer->column++;
        }
    }

    return c;
}

static void lexer_skip_whitespace(Lexer *lexer)
{
    while (isspace((unsigned char)lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }
}

static int lexer_is_alpha(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int lexer_is_digit(char c)
{
    return isdigit((unsigned char)c);
}

static int lexer_is_identifier_char(char c)
{
    return lexer_is_alpha(c) || lexer_is_digit(c);
}

static Token lexer_make_token(Lexer *lexer, TokenType type)
{
    Token token;

    token.type = type;
    token.literal = lexer->source + lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->start_line;
    token.column = lexer->start_column;

    return token;
}

static TokenType lexer_keyword_type(const char *start, size_t length)
{
    if (length == 3 && start[0] == 's' &&
        start[1] == 'e' && start[2] == 't')
    {
        return TOKEN_SET;
    }

    if (length == 4 && start[0] == 'l' &&
        start[1] == 'o' && start[2] == 'c' &&
        start[3] == 'k')
    {
        return TOKEN_LOCK;
    }

    if (length == 2 && start[0] == 'i' && start[1] == 'f')
    {
        return TOKEN_IF;
    }

    if (length == 4 && start[0] == 'e' &&
        start[1] == 'l' && start[2] == 's' &&
        start[3] == 'e')
    {
        return TOKEN_ELSE;
    }

    if (length == 4 && start[0] == 't' &&
        start[1] == 'r' && start[2] == 'u' &&
        start[3] == 'e')
    {
        return TOKEN_TRUE;
    }

    if (length == 5 && start[0] == 'f' &&
        start[1] == 'a' && start[2] == 'l' &&
        start[3] == 's' && start[4] == 'e')
    {
        return TOKEN_FALSE;
    }

    if (length == 3 && start[0] == 'i' &&
        start[1] == 'n' && start[2] == 't')
    {
        return TOKEN_TYPE_INT;
    }

    if (length == 5 && start[0] == 'f' &&
        start[1] == 'l' && start[2] == 'o' &&
        start[3] == 'a' && start[4] == 't')
    {
        return TOKEN_TYPE_FLOAT;
    }

    if (length == 4 && start[0] == 'b' &&
        start[1] == 'o' && start[2] == 'o' &&
        start[3] == 'l')
    {
        return TOKEN_TYPE_BOOL;
    }

    if (length == 4 && start[0] == 'c' &&
        start[1] == 'h' && start[2] == 'a' &&
        start[3] == 'r')
    {
        return TOKEN_TYPE_CHAR;
    }

    if (length == 3 && start[0] == 's' &&
        start[1] == 't' && start[2] == 'r')
    {
        return TOKEN_TYPE_STR;
    }

    return TOKEN_IDENTIFIER;
}

void lexer_init(Lexer *lexer, const char *source)
{
    lexer->source = source;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->start_line = 1;
    lexer->start_column = 1;
}

Token lexer_next_token(Lexer *lexer)
{
    lexer_skip_whitespace(lexer);

    lexer->start = lexer->current;
    lexer->start_line = lexer->line;
    lexer->start_column = lexer->column;

    char c = lexer_peek(lexer);

    if (c == '\0')
    {
        return lexer_make_token(lexer, TOKEN_EOF);
    }

    if (lexer_is_alpha(c))
    {
        lexer_advance(lexer);

        while (lexer_is_identifier_char(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }

        return lexer_make_token(
            lexer,
            lexer_keyword_type(
                lexer->source + lexer->start,
                lexer->current - lexer->start));
    }

    if (lexer_is_digit(c))
    {
        lexer_advance(lexer);

        while (lexer_is_digit(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }

        return lexer_make_token(lexer, TOKEN_INTEGER);
    }

    if (c == '"')
    {
        lexer_advance(lexer);

        while (lexer_peek(lexer) != '"' &&
               lexer_peek(lexer) != '\0')
        {
            lexer_advance(lexer);
        }

        if (lexer_peek(lexer) == '\0')
        {
            return lexer_make_token(lexer, TOKEN_INVALID);
        }

        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_STRING);
    }

    switch (c)
    {
    case '=':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_EQUAL);
        }

        return lexer_make_token(lexer, TOKEN_ASSIGN);

    case '!':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_NOT_EQUAL);
        }

        return lexer_make_token(lexer, TOKEN_LOGICAL_NOT);

    case '&':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '&')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_LOGICAL_AND);
        }

        return lexer_make_token(lexer, TOKEN_INVALID);

    case '|':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '|')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_LOGICAL_OR);
        }

        return lexer_make_token(lexer, TOKEN_INVALID);

    case '>':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_GREATER_EQUAL);
        }

        return lexer_make_token(lexer, TOKEN_GREATER);

    case '<':
        lexer_advance(lexer);

        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_LESS_EQUAL);
        }

        return lexer_make_token(lexer, TOKEN_LESS);

    case ':':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_COLON);

    case '+':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_PLUS);

    case '-':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_MINUS);

    case '*':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_MULTIPLY);

    case '/':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_DIVIDE);

    case '(':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_LPAREN);

    case ')':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_RPAREN);

    case '{':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_LBRACE);

    case '}':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_RBRACE);

    case ';':
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_SEMICOLON);

    default:
        lexer_advance(lexer);
        return lexer_make_token(lexer, TOKEN_INVALID);
    }
}
