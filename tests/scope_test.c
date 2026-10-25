
#include <stdio.h>

#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"

static int add_statement(ASTNode *block, ASTNode *statement)
{
    if (block == NULL || statement == NULL)
    {
        ast_free(statement);
        return 0;
    }

    if (!ast_block_add_statement(block, statement))
    {
        ast_free(statement);
        return 0;
    }

    return 1;
}

static int run_test(
    const char *name,
    ASTNode *node,
    SymbolTable *table,
    int expected)
{
    if (node == NULL)
    {
        fprintf(stderr, "Falha: AST invalida no teste '%s'.\n", name);
        return 0;
    }

    int result = semantic_analyze(node, table);
    ast_free(node);

    if (result != expected)
    {
        fprintf(stderr, "Falha no teste: %s.\n", name);
        return 0;
    }

    printf("Teste aprovado: %s.\n", name);
    return 1;
}

static int check_integer(
    const char *name,
    const SymbolTable *table,
    const char *variable,
    long expected)
{
    const Symbol *symbol = symbol_table_find(table, variable);

    if (symbol == NULL ||
        symbol->type != AST_TYPE_INT ||
        symbol->integer_value != expected)
    {
        fprintf(stderr, "Falha: valor inesperado em '%s'.\n", name);
        return 0;
    }

    return 1;
}

int main(void)
{
    SymbolTable table;
    symbol_table_init(&table);

    ASTNode *declaration =
        ast_new_variable_declaration("externa", AST_TYPE_INT, ast_new_integer(10));

    if (!run_test("declaracao externa", declaration, &table, 1))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 1: o bloco pode modificar uma variável externa. */
    ASTNode *block = ast_new_block();

    if (!add_statement(block, ast_new_assignment("externa", ast_new_integer(20))))
    {
        ast_free(block);
        symbol_table_free(&table);
        return 1;
    }

    if (!run_test("atribuicao no escopo externo", block, &table, 1) ||
        !check_integer("valor externo modificado", &table, "externa", 20))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 2: uma declaração local desaparece após o bloco. */
    block = ast_new_block();

    if (!add_statement(
            block,
            ast_new_variable_declaration("local", AST_TYPE_INT, ast_new_integer(30))))
    {
        ast_free(block);
        symbol_table_free(&table);
        return 1;
    }

    if (!run_test("declaracao local no bloco", block, &table, 1))
    {
        symbol_table_free(&table);
        return 1;
    }

    ASTNode *reference = ast_new_identifier("local");

    if (!run_test("variavel local invisivel fora do bloco", reference, &table, 0))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 3: shadowing não modifica a variável externa. */
    block = ast_new_block();

    if (!add_statement(
            block,
            ast_new_variable_declaration("externa", AST_TYPE_INT, ast_new_integer(99))) ||
        !add_statement(
            block,
            ast_new_assignment("externa", ast_new_integer(100))))
    {
        ast_free(block);
        symbol_table_free(&table);
        return 1;
    }

    if (!run_test("shadowing no bloco", block, &table, 1) ||
        !check_integer("valor externo preservado", &table, "externa", 20))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 4: um bloco interno pode ler uma variável externa. */
    block = ast_new_block();

    if (!add_statement(
            block,
            ast_new_variable_declaration(
                "copia",
                AST_TYPE_INT,
                ast_new_identifier("externa"))))
    {
        ast_free(block);
        symbol_table_free(&table);
        return 1;
    }

    if (!run_test("leitura de variavel externa", block, &table, 1))
    {
        symbol_table_free(&table);
        return 1;
    }

    symbol_table_free(&table);

    printf("\nTodos os testes de escopo foram concluidos.\n");
    return 0;
}
