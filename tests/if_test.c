
#include <stdio.h>

#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"

static int run_test(
    const char *name,
    ASTNode *node,
    SymbolTable *table,
    int expected_result)
{
    if (node == NULL)
    {
        fprintf(stderr, "Falha: nao foi possivel criar AST para %s.\n", name);
        return 0;
    }

    int result = semantic_analyze(node, table);
    ast_free(node);

    if (result != expected_result)
    {
        fprintf(stderr, "Falha no teste: %s.\n", name);
        return 0;
    }

    printf("Teste aprovado: %s.\n", name);
    return 1;
}

static int check_integer(
    const char *name,
    SymbolTable *table,
    const char *variable,
    int expected_value)
{
    const Symbol *symbol = symbol_table_find(table, variable);

    if (symbol == NULL || symbol->integer_value != expected_value)
    {
        fprintf(stderr, "Falha: valor incorreto de %s apos %s.\n",
                variable, name);
        return 0;
    }

    return 1;
}

static ASTNode *make_assignment(const char *name, int value)
{
    return ast_new_assignment(name, ast_new_integer(value));
}

int main(void)
{
    SymbolTable table;
    symbol_table_init(&table);

    ASTNode *declaration = ast_new_variable_declaration(
        "resultado", AST_TYPE_INT, ast_new_integer(0));

    if (!run_test("declaracao inicial", declaration, &table, 1))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 1: condicao verdadeira executa o ramo then. */
    ASTNode *conditional = ast_new_if(
        ast_new_boolean(1),
        make_assignment("resultado", 10),
        make_assignment("resultado", 20));

    if (!run_test("if com condicao verdadeira", conditional, &table, 1) ||
        !check_integer("if verdadeiro", &table, "resultado", 10))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 2: condicao falsa executa o ramo else. */
    conditional = ast_new_if(
        ast_new_boolean(0),
        make_assignment("resultado", 30),
        make_assignment("resultado", 40));

    if (!run_test("if com condicao falsa", conditional, &table, 1) ||
        !check_integer("if falso", &table, "resultado", 40))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 3: if sem else nao altera o valor quando falso. */
    conditional = ast_new_if(
        ast_new_boolean(0),
        make_assignment("resultado", 50),
        NULL);

    if (!run_test("if sem else com condicao falsa", conditional, &table, 1) ||
        !check_integer("if sem else", &table, "resultado", 40))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 4: condicional aninhada seleciona o ramo correto. */
    ASTNode *inner_if = ast_new_if(
        ast_new_boolean(0),
        make_assignment("resultado", 60),
        make_assignment("resultado", 70));

    conditional = ast_new_if(
        ast_new_boolean(1),
        inner_if,
        make_assignment("resultado", 80));

    if (!run_test("if aninhado", conditional, &table, 1) ||
        !check_integer("if aninhado", &table, "resultado", 70))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 5: condicao inteira deve ser rejeitada. */
    conditional = ast_new_if(
        ast_new_integer(1),
        make_assignment("resultado", 90),
        NULL);

    if (!run_test("rejeicao de condicao nao booleana",
                  conditional, &table, 0))
    {
        symbol_table_free(&table);
        return 1;
    }

    if (!check_integer("condicao invalida", &table, "resultado", 70))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 6: rejeitar variavel inexistente no ramo nao selecionado. */
    conditional = ast_new_if(
        ast_new_boolean(1),
        make_assignment("resultado", 100),
        make_assignment("variavel_inexistente", 200));

    if (!run_test(
            "rejeicao de variavel inexistente no ramo nao selecionado",
            conditional,
            &table,
            0))
    {
        symbol_table_free(&table);
        return 1;
    }

    if (!check_integer(
            "ramo nao selecionado invalido",
            &table,
            "resultado",
            70))
    {
        symbol_table_free(&table);
        return 1;
    }

    /* Teste 7: validar o ramo nao selecionado sem executar sua atribuicao. */
    conditional = ast_new_if(
        ast_new_boolean(1),
        make_assignment("resultado", 80),
        make_assignment("resultado", 90));

    if (!run_test(
            "validacao sem executar ramo nao selecionado",
            conditional,
            &table,
            1) ||
        !check_integer(
            "execucao exclusiva do ramo selecionado",
            &table,
            "resultado",
            80))
    {
        symbol_table_free(&table);
        return 1;
    }

    symbol_table_free(&table);

    printf("\nTodos os testes de condicionais foram concluidos.\n");
    return 0;
}