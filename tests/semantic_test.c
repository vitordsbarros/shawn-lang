
#include <stdio.h>

#include "ast.h"
#include "semantic.h"
#include "symbol_table.h"

int main(void)
{
    SymbolTable table;
    symbol_table_init(&table);

    /* Teste 1: declarar e atribuir uma variavel inteira */
    ASTNode *declaration = ast_new_variable_declaration(
        "idade",
        AST_TYPE_INT,
        ast_new_integer(20));

    if (declaration == NULL)
    {
        fprintf(stderr, "Falha ao criar declaracao de variavel.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (!semantic_analyze(declaration, &table))
    {
        fprintf(stderr, "Erro: declaracao de variavel foi rejeitada.\n");
        ast_free(declaration);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(declaration);

    ASTNode *assignment = ast_new_assignment(
        "idade",
        ast_new_integer(25));

    if (assignment == NULL)
    {
        fprintf(stderr, "Falha ao criar atribuicao.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (!semantic_analyze(assignment, &table))
    {
        fprintf(stderr, "Erro: atribuicao valida foi rejeitada.\n");
        ast_free(assignment);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(assignment);

    const Symbol *idade = symbol_table_find(&table, "idade");

    if (idade == NULL || idade->integer_value != 25)
    {
        fprintf(stderr, "Erro: valor de idade incorreto.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste 1 aprovado: declaracao e atribuicao de variavel.\n");

    /* Teste 2: expressao aritmetica com identificadores */
    ASTNode *bonus_declaration = ast_new_variable_declaration(
        "bonus",
        AST_TYPE_INT,
        ast_new_integer(5));

    if (bonus_declaration == NULL)
    {
        fprintf(stderr, "Falha ao criar declaracao de bonus.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (!semantic_analyze(bonus_declaration, &table))
    {
        fprintf(stderr, "Erro: declaracao de bonus foi rejeitada.\n");
        ast_free(bonus_declaration);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(bonus_declaration);

    ASTNode *sum = ast_new_binary(
        AST_OP_ADD,
        ast_new_identifier("idade"),
        ast_new_identifier("bonus"));

    if (sum == NULL)
    {
        fprintf(stderr, "Falha ao criar expressao de soma.\n");
        symbol_table_free(&table);
        return 1;
    }

    assignment = ast_new_assignment("idade", sum);

    if (assignment == NULL)
    {
        fprintf(stderr, "Falha ao criar atribuicao com soma.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (!semantic_analyze(assignment, &table))
    {
        fprintf(stderr, "Erro: expressao aritmetica foi rejeitada.\n");
        ast_free(assignment);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(assignment);

    idade = symbol_table_find(&table, "idade");

    if (idade == NULL || idade->integer_value != 30)
    {
        fprintf(stderr, "Erro: resultado da soma incorreto.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste 2 aprovado: expressao aritmetica com identificadores.\n");

    /* Teste 3: rejeitar atribuicao de texto a variavel inteira */
    assignment = ast_new_assignment(
        "idade",
        ast_new_string("vinte"));

    if (assignment == NULL)
    {
        fprintf(stderr, "Falha ao criar atribuicao de texto.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (semantic_analyze(assignment, &table))
    {
        fprintf(stderr, "Erro: atribuicao de texto foi aceita.\n");
        ast_free(assignment);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(assignment);

    printf("Teste 3 aprovado: incompatibilidade de tipos rejeitada.\n");

    /* Teste 4: declarar uma constante */
    ASTNode *constant_declaration = ast_new_constant_declaration(
        "limite",
        AST_TYPE_INT,
        ast_new_integer(100));

    if (constant_declaration == NULL)
    {
        fprintf(stderr, "Falha ao criar declaracao de constante.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (!semantic_analyze(constant_declaration, &table))
    {
        fprintf(stderr, "Erro: declaracao de constante foi rejeitada.\n");
        ast_free(constant_declaration);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(constant_declaration);

    const Symbol *limite = symbol_table_find(&table, "limite");

    if (limite == NULL || limite->integer_value != 100)
    {
        fprintf(stderr, "Erro: valor inicial da constante incorreto.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste 4 aprovado: declaracao de constante.\n");

    /* Teste 5: rejeitar alteracao de constante */
    assignment = ast_new_assignment(
        "limite",
        ast_new_integer(200));

    if (assignment == NULL)
    {
        fprintf(stderr, "Falha ao criar atribuicao a constante.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (semantic_analyze(assignment, &table))
    {
        fprintf(stderr, "Erro: alteracao de constante foi aceita.\n");
        ast_free(assignment);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(assignment);

    limite = symbol_table_find(&table, "limite");

    if (limite == NULL || limite->integer_value != 100)
    {
        fprintf(stderr, "Erro: valor da constante foi alterado.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf("Teste 5 aprovado: alteracao de constante rejeitada.\n");

    /* Teste 6: rejeitar divisao por zero na inicializacao */
    ASTNode *division_by_zero_declaration =
        ast_new_variable_declaration(
            "resultado",
            AST_TYPE_INT,
            ast_new_binary(
                AST_OP_DIVIDE,
                ast_new_integer(10),
                ast_new_integer(0)));

    if (division_by_zero_declaration == NULL)
    {
        fprintf(
            stderr,
            "Falha ao criar declaracao com divisao por zero.\n");
        symbol_table_free(&table);
        return 1;
    }

    if (semantic_analyze(division_by_zero_declaration, &table))
    {
        fprintf(
            stderr,
            "Erro: foi aceita uma divisao por zero.\n");
        ast_free(division_by_zero_declaration);
        symbol_table_free(&table);
        return 1;
    }

    ast_free(division_by_zero_declaration);

    if (symbol_table_find(&table, "resultado") != NULL)
    {
        fprintf(
            stderr,
            "Erro: a variavel resultado foi registrada apos uma falha.\n");
        symbol_table_free(&table);
        return 1;
    }

    printf(
        "Teste 6 aprovado: divisao por zero rejeitada sem registrar variavel.\n");

    symbol_table_free(&table);

    printf("\nTodos os testes semanticos foram concluidos.\n");

    return 0;
}
