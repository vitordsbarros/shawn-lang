#include <stdio.h>

#include "ast.h"

static int test_integer_evaluation(void)
{
    ASTNode *expression = ast_new_binary(
        AST_OP_ADD,
        ast_new_integer(10),
        ast_new_binary(
            AST_OP_MULTIPLY,
            ast_new_integer(5),
            ast_new_integer(2)));

    if (expression == NULL)
    {
        fprintf(stderr, "Erro ao criar a expressao inteira.\n");
        return 0;
    }

    long result;

    if (!ast_evaluate_integer(expression, &result))
    {
        fprintf(stderr, "Erro ao avaliar a expressao inteira.\n");
        ast_free(expression);
        return 0;
    }

    ast_free(expression);

    if (result != 20)
    {
        fprintf(stderr, "Falha: esperava 20, obteve %ld.\n", result);
        return 0;
    }

    printf("Teste aprovado: avaliacao aritmetica.\n");
    return 1;
}

static int test_boolean_evaluation(
    const char *name,
    ASTNode *expression,
    int expected)
{
    if (expression == NULL)
    {
        fprintf(stderr, "Erro ao criar a expressao: %s.\n", name);
        return 0;
    }

    int result;

    if (!ast_evaluate_boolean(expression, &result))
    {
        fprintf(stderr, "Erro ao avaliar a expressao: %s.\n", name);
        ast_free(expression);
        return 0;
    }

    ast_free(expression);

    if (result != expected)
    {
        fprintf(
            stderr,
            "Falha em '%s': esperava %s, obteve %s.\n",
            name,
            expected ? "true" : "false",
            result ? "true" : "false");
        return 0;
    }

    printf("Teste aprovado: %s.\n", name);
    return 1;
}

int main(void)
{
    if (!test_integer_evaluation())
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "!false",
            ast_new_unary(
                AST_UNARY_LOGICAL_NOT,
                ast_new_boolean(0)),
            1))
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "10 > 5",
            ast_new_binary(
                AST_OP_GREATER,
                ast_new_integer(10),
                ast_new_integer(5)),
            1))
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "true && false",
            ast_new_binary(
                AST_OP_LOGICAL_AND,
                ast_new_boolean(1),
                ast_new_boolean(0)),
            0))
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "true || false",
            ast_new_binary(
                AST_OP_LOGICAL_OR,
                ast_new_boolean(1),
                ast_new_boolean(0)),
            1))
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "!(10 < 5) && true",
            ast_new_binary(
                AST_OP_LOGICAL_AND,
                ast_new_unary(
                    AST_UNARY_LOGICAL_NOT,
                    ast_new_binary(
                        AST_OP_LESS,
                        ast_new_integer(10),
                        ast_new_integer(5))),
                ast_new_boolean(1)),
            1))
    {
        return 1;
    }

    if (!test_boolean_evaluation(
            "2 + 3 * 4 > 10",
            ast_new_binary(
                AST_OP_GREATER,
                ast_new_binary(
                    AST_OP_ADD,
                    ast_new_integer(2),
                    ast_new_binary(
                        AST_OP_MULTIPLY,
                        ast_new_integer(3),
                        ast_new_integer(4))),
                ast_new_integer(10)),
            1))
    {
        return 1;
    }

    /*
     * Curto-circuito:
     * false && (1 / 0 == 0) deve resultar em false
     * sem avaliar a expressao da direita.
     */
    if (!test_boolean_evaluation(
            "false && (1 / 0 == 0)",
            ast_new_binary(
                AST_OP_LOGICAL_AND,
                ast_new_boolean(0),
                ast_new_binary(
                    AST_OP_EQUAL,
                    ast_new_binary(
                        AST_OP_DIVIDE,
                        ast_new_integer(1),
                        ast_new_integer(0)),
                    ast_new_integer(0))),
            0))
    {
        return 1;
    }

    /*
     * Curto-circuito:
     * true || (1 / 0 == 0) deve resultar em true
     * sem avaliar a expressao da direita.
     */
    if (!test_boolean_evaluation(
            "true || (1 / 0 == 0)",
            ast_new_binary(
                AST_OP_LOGICAL_OR,
                ast_new_boolean(1),
                ast_new_binary(
                    AST_OP_EQUAL,
                    ast_new_binary(
                        AST_OP_DIVIDE,
                        ast_new_integer(1),
                        ast_new_integer(0)),
                    ast_new_integer(0))),
            1))
    {
        return 1;
    }

    printf("Todos os testes de avaliacao da AST foram concluidos.\n");
    return 0;
}