#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"

static int analyze_source(const char *source, SymbolTable *table)
{
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    ASTNode *root = parser_parse_statement(&parser);

    if (root == NULL || parser.has_error)
    {
        fprintf(stderr, "Erro ao analisar a instrucao.\n");
        ast_free(root);
        return 0;
    }

    int success = semantic_analyze(root, table);

    if (success)
    {
        ast_print(root);
    }

    ast_free(root);
    return success;
}

static int test_logical_operator(
    const char *source,
    TokenType expected_type)
{
    Lexer lexer;
    lexer_init(&lexer, source);

    Token token = lexer_next_token(&lexer);

    if (token.type != expected_type || token.length != 2)
    {
        fprintf(
            stderr,
            "Falha no teste do operador '%s': token inesperado.\n",
            source);
        return 0;
    }

    Token next = lexer_next_token(&lexer);

    if (next.type != TOKEN_EOF)
    {
        fprintf(
            stderr,
            "Falha no teste do operador '%s': token extra.\n",
            source);
        return 0;
    }

    return 1;
}

static int test_logical_not(void)
{
    Lexer lexer;
    lexer_init(&lexer, "!");

    Token token = lexer_next_token(&lexer);

    if (token.type != TOKEN_LOGICAL_NOT || token.length != 1)
    {
        fprintf(stderr, "Falha no teste do operador '!'.\n");
        return 0;
    }

    if (lexer_next_token(&lexer).type != TOKEN_EOF)
    {
        fprintf(stderr, "Falha: token extra depois de '!'.\n");
        return 0;
    }

    return 1;
}

static int test_invalid_single_operator(const char *source)
{
    Lexer lexer;
    lexer_init(&lexer, source);

    Token token = lexer_next_token(&lexer);

    if (token.type != TOKEN_INVALID || token.length != 1)
    {
        fprintf(
            stderr,
            "Falha: o operador isolado '%s' deveria ser invalido.\n",
            source);
        return 0;
    }

    return 1;
}

static int test_expression(
    const char *source,
    ASTType expected_type)
{
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    ASTNode *root = parser_parse_expression(&parser);

    if (root == NULL || parser.has_error)
    {
        fprintf(
            stderr,
            "Falha: a expressao '%s' deveria ser aceita pelo parser.\n",
            source);
        ast_free(root);
        return 0;
    }

    ASTType actual_type = ast_infer_type(root);

    if (actual_type != expected_type)
    {
        fprintf(
            stderr,
            "Falha: tipo incorreto para a expressao '%s'.\n",
            source);
        ast_free(root);
        return 0;
    }

    ast_free(root);
    return 1;
}

static int test_boolean_expression(
    const char *source,
    SymbolTable *table,
    int expected_success)
{
    ASTNode *root = NULL;
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    root = parser_parse_expression(&parser);

    if (root == NULL || parser.has_error)
    {
        if (expected_success)
        {
            fprintf(
                stderr,
                "Falha: o parser rejeitou a expressao '%s'.\n",
                source);
            ast_free(root);
            return 0;
        }

        ast_free(root);
        return 1;
    }

    int success = semantic_analyze(root, table);

    if (success != expected_success)
    {
        fprintf(
            stderr,
            "Falha: resultado semantico inesperado para '%s'.\n",
            source);
        ast_free(root);
        return 0;
    }

    ast_free(root);
    return 1;
}

int main(void)
{
    SymbolTable table;
    symbol_table_init(&table);

    if (!analyze_source("set total = 10;", &table))
    {
        symbol_table_free(&table);
        return 1;
    }

    if (analyze_source("set total = 20;", &table))
    {
        fprintf(stderr, "Erro: a declaracao duplicada foi aceita.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste concluido: declaracao duplicada rejeitada.\n");

    if (!test_logical_operator("&&", TOKEN_LOGICAL_AND) ||
        !test_logical_operator("||", TOKEN_LOGICAL_OR) ||
        !test_logical_operator("!=", TOKEN_NOT_EQUAL) ||
        !test_logical_not() ||
        !test_invalid_single_operator("&") ||
        !test_invalid_single_operator("|"))
    {
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste concluido: operadores logicos do lexer.\n");

    if (!test_expression("true", AST_TYPE_BOOL) ||
        !test_expression("false", AST_TYPE_BOOL) ||
        !test_expression("!false", AST_TYPE_BOOL) ||
        !test_expression("10 > 5", AST_TYPE_BOOL) ||
        !test_expression("10 >= 10", AST_TYPE_BOOL) ||
        !test_expression("3 < 5", AST_TYPE_BOOL) ||
        !test_expression("5 <= 5", AST_TYPE_BOOL) ||
        !test_expression("10 == 10", AST_TYPE_BOOL) ||
        !test_expression("10 != 5", AST_TYPE_BOOL) ||
        !test_expression("true && false", AST_TYPE_BOOL) ||
        !test_expression("true || false", AST_TYPE_BOOL) ||
        !test_expression("!(10 < 5) && true", AST_TYPE_BOOL) ||
        !test_expression("2 + 3 * 4 > 10", AST_TYPE_BOOL))
    {
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste concluido: parser de expressoes booleanas.\n");

    if (!test_boolean_expression("true && 10", &table, 0) ||
        !test_boolean_expression("!10", &table, 0) ||
        !test_boolean_expression("10 && true", &table, 0) ||
        !test_boolean_expression("true || 10", &table, 0) ||
        !test_boolean_expression("10 == true", &table, 0))
    {
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste concluido: rejeicao semantica de operandos incompatíveis.\n");

    symbol_table_free(&table);
    return 0;
}