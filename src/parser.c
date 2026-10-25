#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/* Avança para o próximo token. */
static void parser_advance(Parser *parser)
{
    parser->current_token = lexer_next_token(parser->lexer);
}

/* Cria uma string terminada em '\0' a partir de um token. */
static char *parser_copy_token_text(const Token *token)
{
    char *text = malloc(token->length + 1);

    if (text == NULL)
    {
        return NULL;
    }

    memcpy(text, token->literal, token->length);
    text[token->length] = '\0';

    return text;
}

/* Inicializa o parser e carrega o primeiro token. */
void parser_init(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;
    parser->has_error = 0;
    parser->current_token = lexer_next_token(lexer);
}

/* Declarações antecipadas das funções de análise. */
static ASTNode *parser_parse_expression_internal(Parser *parser);
static ASTNode *parser_parse_term(Parser *parser);
static ASTNode *parser_parse_factor(Parser *parser);

/* Converte um token de tipo no tipo correspondente da AST. */
static ASTType parser_parse_type(TokenType token_type)
{
    switch (token_type)
    {
    case TOKEN_TYPE_INT:
        return AST_TYPE_INT;

    case TOKEN_TYPE_FLOAT:
        return AST_TYPE_FLOAT;

    case TOKEN_TYPE_BOOL:
        return AST_TYPE_BOOL;

    case TOKEN_TYPE_CHAR:
        return AST_TYPE_CHAR;

    case TOKEN_TYPE_STR:
        return AST_TYPE_STR;

    default:
        return AST_TYPE_INFERRED;
    }
}

/* Analisa inteiros, identificadores e expressões entre parênteses. */
static ASTNode *parser_parse_factor(Parser *parser)
{
    Token token = parser->current_token;

    if (token.type == TOKEN_INTEGER)
    {
        char *text = parser_copy_token_text(&token);

        if (text == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        errno = 0;
        char *end = NULL;
        long value = strtol(text, &end, 10);

        int invalid =
            errno == ERANGE ||
            end == text ||
            *end != '\0';

        free(text);

        if (invalid)
        {
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        return ast_new_integer(value);
    }

    if (token.type == TOKEN_STRING)
    {
        /* O token inclui as aspas delimitadoras. */
        if (token.length < 2)
        {
            parser->has_error = 1;
            return NULL;
        }

        size_t length = token.length - 2;
        char *text = malloc(length + 1);

        if (text == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        memcpy(text, token.literal + 1, length);
        text[length] = '\0';

        ASTNode *node = ast_new_string(text);
        free(text);

        if (node == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        return node;
    }

    if (token.type == TOKEN_IDENTIFIER)
    {
        char *name = parser_copy_token_text(&token);

        if (name == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        ASTNode *node = ast_new_identifier(name);
        free(name);

        if (node == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        return node;
    }

    if (token.type == TOKEN_LPAREN)
    {
        parser_advance(parser);

        ASTNode *node =
            parser_parse_expression_internal(parser);

        if (node == NULL)
        {
            return NULL;
        }

        if (parser->current_token.type != TOKEN_RPAREN)
        {
            ast_free(node);
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);
        return node;
    }

    parser->has_error = 1;
    return NULL;
}

/* Analisa multiplicação e divisão. */
static ASTNode *parser_parse_term(Parser *parser)
{
    ASTNode *left = parser_parse_factor(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (
        parser->current_token.type == TOKEN_MULTIPLY ||
        parser->current_token.type == TOKEN_DIVIDE)
    {
        TokenType token_type = parser->current_token.type;

        ASTOperator operator =
            token_type == TOKEN_MULTIPLY
                ? AST_OP_MULTIPLY
                : AST_OP_DIVIDE;

        parser_advance(parser);

        ASTNode *right = parser_parse_factor(parser);

        if (right == NULL)
        {
            ast_free(left);
            return NULL;
        }

        ASTNode *combined =
            ast_new_binary(operator, left, right);

        if (combined == NULL)
        {
            ast_free(left);
            ast_free(right);
            parser->has_error = 1;
            return NULL;
        }

        left = combined;
    }

    return left;
}

/* Analisa adição e subtração. */
static ASTNode *parser_parse_expression_internal(Parser *parser)
{
    ASTNode *left = parser_parse_term(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (
        parser->current_token.type == TOKEN_PLUS ||
        parser->current_token.type == TOKEN_MINUS)
    {
        TokenType token_type = parser->current_token.type;

        ASTOperator operator =
            token_type == TOKEN_PLUS
                ? AST_OP_ADD
                : AST_OP_SUBTRACT;

        parser_advance(parser);

        ASTNode *right = parser_parse_term(parser);

        if (right == NULL)
        {
            ast_free(left);
            return NULL;
        }

        ASTNode *combined =
            ast_new_binary(operator, left, right);

        if (combined == NULL)
        {
            ast_free(left);
            ast_free(right);
            parser->has_error = 1;
            return NULL;
        }

        left = combined;
    }

    return left;
}

/* Interface pública para analisar uma expressão. */

ASTNode *parser_parse_expression(Parser *parser)
{
    if (parser == NULL || parser->lexer == NULL)
    {
        return NULL;
    }

    parser->has_error = 0;

    ASTNode *root = parser_parse_expression_internal(parser);

    if (root == NULL)
    {
        return NULL;
    }

    /*
     * Uma expressão isolada deve terminar no EOF.
     * O ponto e vírgula será tratado quando implementarmos
     * a análise de instruções.
     */
    if (parser->current_token.type != TOKEN_EOF)
    {
        ast_free(root);
        parser->has_error = 1;
        return NULL;
    }

    return root;
}

/* Analisa uma atribuição ou declaração de variável/constante. */
ASTNode *parser_parse_statement(Parser *parser)
{
    if (parser == NULL || parser->lexer == NULL)
    {
        return NULL;
    }

    parser->has_error = 0;

    TokenType initial_type = parser->current_token.type;

    /* Declarações começam com set ou lock. */
    if (initial_type == TOKEN_SET || initial_type == TOKEN_LOCK)
    {
        int is_constant = initial_type == TOKEN_LOCK;
        parser_advance(parser);

        ASTType declared_type = AST_TYPE_INFERRED;

        /* O tipo é opcional: int:, str:, etc. */
        switch (parser->current_token.type)
        {
        case TOKEN_TYPE_INT:
        case TOKEN_TYPE_FLOAT:
        case TOKEN_TYPE_BOOL:
        case TOKEN_TYPE_CHAR:
        case TOKEN_TYPE_STR:
            declared_type =
                parser_parse_type(parser->current_token.type);
            parser_advance(parser);

            if (parser->current_token.type != TOKEN_COLON)
            {
                parser->has_error = 1;
                return NULL;
            }

            parser_advance(parser);
            break;

        default:
            break;
        }

        if (parser->current_token.type != TOKEN_IDENTIFIER)
        {
            parser->has_error = 1;
            return NULL;
        }

        char *name = parser_copy_token_text(&parser->current_token);

        if (name == NULL)
        {
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        if (parser->current_token.type != TOKEN_ASSIGN)
        {
            free(name);
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        ASTNode *value = parser_parse_expression_internal(parser);

        if (value == NULL)
        {
            free(name);
            return NULL;
        }

        if (parser->current_token.type != TOKEN_SEMICOLON)
        {
            free(name);
            ast_free(value);
            parser->has_error = 1;
            return NULL;
        }

        parser_advance(parser);

        if (parser->current_token.type != TOKEN_EOF &&
            parser->current_token.type != TOKEN_SET &&
            parser->current_token.type != TOKEN_LOCK &&
            parser->current_token.type != TOKEN_IDENTIFIER)
        {
            free(name);
            ast_free(value);
            parser->has_error = 1;
            return NULL;
        }

        ASTNode *statement;

        if (is_constant)
        {
            statement = ast_new_constant_declaration(
                name, declared_type, value);
        }
        else
        {
            statement = ast_new_variable_declaration(
                name, declared_type, value);
        }

        free(name);

        if (statement == NULL)
        {
            ast_free(value);
            parser->has_error = 1;
            return NULL;
        }

        return statement;
    }

    /* Mantém o suporte à atribuição existente. */
    if (initial_type != TOKEN_IDENTIFIER)
    {
        parser->has_error = 1;
        return NULL;
    }

    char *name = parser_copy_token_text(&parser->current_token);

    if (name == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    if (parser->current_token.type != TOKEN_ASSIGN)
    {
        free(name);
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    ASTNode *value = parser_parse_expression_internal(parser);

    if (value == NULL)
    {
        free(name);
        return NULL;
    }

    if (parser->current_token.type != TOKEN_SEMICOLON)
    {
        free(name);
        ast_free(value);
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    if (parser->current_token.type != TOKEN_EOF &&
        parser->current_token.type != TOKEN_SET &&
        parser->current_token.type != TOKEN_LOCK &&
        parser->current_token.type != TOKEN_IDENTIFIER)
    {
        free(name);
        ast_free(value);
        parser->has_error = 1;
        return NULL;
    }

    ASTNode *statement = ast_new_assignment(name, value);
    free(name);

    if (statement == NULL)
    {
        ast_free(value);
        parser->has_error = 1;
        return NULL;
    }

    return statement;
}