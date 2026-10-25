#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static void parser_advance(Parser *parser)
{
    parser->current_token = lexer_next_token(parser->lexer);
}

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

void parser_init(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;
    parser->has_error = 0;
    parser->current_token = lexer_next_token(lexer);
}

static ASTNode *parser_parse_expression_internal(Parser *parser);
static ASTNode *parser_parse_or(Parser *parser);
static ASTNode *parser_parse_and(Parser *parser);
static ASTNode *parser_parse_equality(Parser *parser);
static ASTNode *parser_parse_comparison(Parser *parser);
static ASTNode *parser_parse_addition(Parser *parser);
static ASTNode *parser_parse_term(Parser *parser);
static ASTNode *parser_parse_unary(Parser *parser);
static ASTNode *parser_parse_factor(Parser *parser);
static ASTNode *parser_parse_block(Parser *parser);
static ASTNode *parser_parse_if(Parser *parser);
static ASTNode *parser_parse_statement_internal(Parser *parser);

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

        ASTNode *node = ast_new_integer(value);

        if (node == NULL)
        {
            parser->has_error = 1;
        }

        return node;
    }

    if (token.type == TOKEN_TRUE || token.type == TOKEN_FALSE)
    {
        int value = token.type == TOKEN_TRUE;

        parser_advance(parser);

        ASTNode *node = ast_new_boolean(value);

        if (node == NULL)
        {
            parser->has_error = 1;
        }

        return node;
    }

    if (token.type == TOKEN_STRING)
    {
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

/* Negacao logica: ! */
static ASTNode *parser_parse_unary(Parser *parser)
{
    if (parser->current_token.type == TOKEN_LOGICAL_NOT)
    {
        parser_advance(parser);

        ASTNode *operand = parser_parse_unary(parser);

        if (operand == NULL)
        {
            return NULL;
        }

        ASTNode *node = ast_new_unary(
            AST_UNARY_LOGICAL_NOT,
            operand);

        if (node == NULL)
        {
            ast_free(operand);
            parser->has_error = 1;
        }

        return node;
    }

    return parser_parse_factor(parser);
}

/* Multiplicacao e divisao: * / */
static ASTNode *parser_parse_term(Parser *parser)
{
    ASTNode *left = parser_parse_unary(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (parser->current_token.type == TOKEN_MULTIPLY ||
           parser->current_token.type == TOKEN_DIVIDE)
    {
        TokenType type = parser->current_token.type;

        ASTOperator operator = type == TOKEN_MULTIPLY
            ? AST_OP_MULTIPLY
            : AST_OP_DIVIDE;

        parser_advance(parser);

        ASTNode *right = parser_parse_unary(parser);

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

/* Adicao e subtracao: + - */
static ASTNode *parser_parse_addition(Parser *parser)
{
    ASTNode *left = parser_parse_term(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (parser->current_token.type == TOKEN_PLUS ||
           parser->current_token.type == TOKEN_MINUS)
    {
        TokenType type = parser->current_token.type;

        ASTOperator operator = type == TOKEN_PLUS
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

static int parser_is_comparison_operator(TokenType type)
{
    return type == TOKEN_GREATER ||
           type == TOKEN_GREATER_EQUAL ||
           type == TOKEN_LESS ||
           type == TOKEN_LESS_EQUAL;
}

static ASTOperator parser_comparison_operator(TokenType type)
{
    switch (type)
    {
    case TOKEN_GREATER:
        return AST_OP_GREATER;
    case TOKEN_GREATER_EQUAL:
        return AST_OP_GREATER_EQUAL;
    case TOKEN_LESS:
        return AST_OP_LESS;
    default:
        return AST_OP_LESS_EQUAL;
    }
}

/* Comparacoes relacionais: > >= < <= */
static ASTNode *parser_parse_comparison(Parser *parser)
{
    ASTNode *left = parser_parse_addition(parser);

    if (left == NULL)
    {
        return NULL;
    }

    if (parser_is_comparison_operator(parser->current_token.type))
    {
        ASTOperator operator =
            parser_comparison_operator(parser->current_token.type);

        parser_advance(parser);

        ASTNode *right = parser_parse_addition(parser);

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

        if (parser_is_comparison_operator(parser->current_token.type))
        {
            ast_free(left);
            parser->has_error = 1;
            return NULL;
        }
    }

    return left;
}

/* Igualdade: == != */
static ASTNode *parser_parse_equality(Parser *parser)
{
    ASTNode *left = parser_parse_comparison(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (parser->current_token.type == TOKEN_EQUAL ||
           parser->current_token.type == TOKEN_NOT_EQUAL)
    {
        TokenType type = parser->current_token.type;

        ASTOperator operator = type == TOKEN_EQUAL
            ? AST_OP_EQUAL
            : AST_OP_NOT_EQUAL;

        parser_advance(parser);

        ASTNode *right = parser_parse_comparison(parser);

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

/* AND logico: && */
static ASTNode *parser_parse_and(Parser *parser)
{
    ASTNode *left = parser_parse_equality(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (parser->current_token.type == TOKEN_LOGICAL_AND)
    {
        parser_advance(parser);

        ASTNode *right = parser_parse_equality(parser);

        if (right == NULL)
        {
            ast_free(left);
            return NULL;
        }

        ASTNode *combined =
            ast_new_binary(AST_OP_LOGICAL_AND, left, right);

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

/* OR logico: || */
static ASTNode *parser_parse_or(Parser *parser)
{
    ASTNode *left = parser_parse_and(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (parser->current_token.type == TOKEN_LOGICAL_OR)
    {
        parser_advance(parser);

        ASTNode *right = parser_parse_and(parser);

        if (right == NULL)
        {
            ast_free(left);
            return NULL;
        }

        ASTNode *combined =
            ast_new_binary(AST_OP_LOGICAL_OR, left, right);

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

static ASTNode *parser_parse_expression_internal(Parser *parser)
{
    return parser_parse_or(parser);
}

/*
 * Analisa um bloco:
 *
 * {
 *     statement;
 *     if (...) { ... }
 * }
 */
static ASTNode *parser_parse_block(Parser *parser)
{
    if (parser->current_token.type != TOKEN_LBRACE)
    {
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    ASTNode *block = ast_new_block();

    if (block == NULL)
    {
        parser->has_error = 1;
        return NULL;
    }

    while (parser->current_token.type != TOKEN_RBRACE &&
           parser->current_token.type != TOKEN_EOF)
    {
        ASTNode *statement = parser_parse_statement_internal(parser);

        if (statement == NULL)
        {
            ast_free(block);
            return NULL;
        }

        if (!ast_block_add_statement(block, statement))
        {
            ast_free(statement);
            ast_free(block);
            parser->has_error = 1;
            return NULL;
        }
    }

    if (parser->current_token.type != TOKEN_RBRACE)
    {
        ast_free(block);
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);
    return block;
}

/* Analisa if (condicao) { ... } [else { ... }]. */
static ASTNode *parser_parse_if(Parser *parser)
{
    if (parser->current_token.type != TOKEN_IF)
    {
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    if (parser->current_token.type != TOKEN_LPAREN)
    {
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    ASTNode *condition = parser_parse_expression_internal(parser);

    if (condition == NULL)
    {
        return NULL;
    }

    if (parser->current_token.type != TOKEN_RPAREN)
    {
        ast_free(condition);
        parser->has_error = 1;
        return NULL;
    }

    parser_advance(parser);

    ASTNode *then_branch = parser_parse_block(parser);

    if (then_branch == NULL)
    {
        ast_free(condition);
        return NULL;
    }

    ASTNode *else_branch = NULL;

    if (parser->current_token.type == TOKEN_ELSE)
    {
        parser_advance(parser);

        else_branch = parser_parse_block(parser);

        if (else_branch == NULL)
        {
            ast_free(condition);
            ast_free(then_branch);
            return NULL;
        }
    }

    ASTNode *node = ast_new_if(condition, then_branch, else_branch);

    if (node == NULL)
    {
        ast_free(condition);
        ast_free(then_branch);
        ast_free(else_branch);
        parser->has_error = 1;
        return NULL;
    }

    return node;
}

/* Declaracoes, atribuicoes e condicionais. */
static ASTNode *parser_parse_statement_internal(Parser *parser)
{
    TokenType initial_type = parser->current_token.type;

    if (initial_type == TOKEN_IF)
    {
        return parser_parse_if(parser);
    }

    if (initial_type == TOKEN_SET || initial_type == TOKEN_LOCK)
    {
        int is_constant = initial_type == TOKEN_LOCK;
        parser_advance(parser);

        ASTType declared_type = AST_TYPE_INFERRED;

        switch (parser->current_token.type)
        {
        case TOKEN_TYPE_INT:
        case TOKEN_TYPE_FLOAT:
        case TOKEN_TYPE_BOOL:
        case TOKEN_TYPE_CHAR:
        case TOKEN_TYPE_STR:
            declared_type = parser_parse_type(parser->current_token.type);
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

        ASTNode *statement = is_constant
            ? ast_new_constant_declaration(name, declared_type, value)
            : ast_new_variable_declaration(name, declared_type, value);

        free(name);

        if (statement == NULL)
        {
            ast_free(value);
            parser->has_error = 1;
            return NULL;
        }

        return statement;
    }

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

/* Interface publica: expressao isolada deve terminar no EOF. */
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

    if (parser->current_token.type != TOKEN_EOF)
    {
        ast_free(root);
        parser->has_error = 1;
        return NULL;
    }

    return root;
}

/* Interface publica: analisa uma unica instrucao por chamada. */
ASTNode *parser_parse_statement(Parser *parser)
{
    if (parser == NULL || parser->lexer == NULL)
    {
        return NULL;
    }

    parser->has_error = 0;

    return parser_parse_statement_internal(parser);
}