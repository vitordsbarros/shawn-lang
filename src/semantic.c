
#include <stdio.h>

#include "semantic.h"

static ASTType semantic_infer_type(const ASTNode *node, const SymbolTable *table)
{
    if (node == NULL || table == NULL)
    {
        return AST_TYPE_INFERRED;
    }

    switch (node->type)
    {
    case AST_IDENTIFIER:
    {
        const Symbol *symbol =
            symbol_table_find(table, node->as.identifier.name);

        if (symbol == NULL)
        {
            return AST_TYPE_INFERRED;
        }

        return symbol->type;
    }

    case AST_BINARY_EXPR:
    {
        ASTType left_type =
            semantic_infer_type(node->as.binary.left, table);
        ASTType right_type =
            semantic_infer_type(node->as.binary.right, table);

        if (left_type == AST_TYPE_INT && right_type == AST_TYPE_INT)
        {
            return AST_TYPE_INT;
        }

        return AST_TYPE_INFERRED;
    }

    default:
        return ast_infer_type(node);
    }
}

static int semantic_evaluate_integer(
    const ASTNode *node,
    const SymbolTable *table,
    long *result,
    int *division_by_zero)
{
    if (node == NULL || table == NULL || result == NULL)
    {
        return 0;
    }

    if (node->type == AST_IDENTIFIER)
    {
        const Symbol *symbol =
            symbol_table_find(table, node->as.identifier.name);

        if (symbol == NULL || symbol->type != AST_TYPE_INT)
        {
            return 0;
        }

        *result = symbol->integer_value;
        return 1;
    }

    if (node->type == AST_INTEGER)
    {
        *result = node->as.integer_value;
        return 1;
    }

    if (node->type != AST_BINARY_EXPR)
    {
        return 0;
    }

    long left;
    long right;

    if (!semantic_evaluate_integer(
            node->as.binary.left,
            table,
            &left,
            division_by_zero) ||
        !semantic_evaluate_integer(
            node->as.binary.right,
            table,
            &right,
            division_by_zero))
    {
        return 0;
    }

    switch (node->as.binary.operator)
    {
    case AST_OP_ADD:
        *result = left + right;
        return 1;

    case AST_OP_SUBTRACT:
        *result = left - right;
        return 1;

    case AST_OP_MULTIPLY:
        *result = left * right;
        return 1;

    case AST_OP_DIVIDE:
        if (right == 0)
        {
            if (division_by_zero != NULL)
            {
                *division_by_zero = 1;
            }

            return 0;
        }

        *result = left / right;
        return 1;

    default:
        return 0;
    }
}

int semantic_analyze(ASTNode *node, SymbolTable *table)
{
    if (node == NULL || table == NULL)
    {
        return 0;
    }

    switch (node->type)
    {

    case AST_VARIABLE_DECLARATION:
    {
        const char *name = node->as.variable_declaration.name;
        ASTType declared_type = node->as.variable_declaration.declared_type;
        ASTType actual_type =
            semantic_infer_type(node->as.variable_declaration.value, table);

        if (declared_type != AST_TYPE_INFERRED &&
            declared_type != actual_type)
        {
            fprintf(
                stderr,
                "Erro semantico: a variavel '%s' foi declarada como '%s', "
                "mas recebeu um valor do tipo '%s'.\n",
                name,
                ast_type_name(declared_type),
                ast_type_name(actual_type));
            return 0;
        }

        ASTType final_type =
            declared_type == AST_TYPE_INFERRED
                ? actual_type
                : declared_type;

        if (final_type == AST_TYPE_INFERRED)
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inferir o tipo "
                "da variavel '%s'.\n",
                name);
            return 0;
        }

        long value = 0;

        if (final_type == AST_TYPE_INT)
        {
            int division_by_zero = 0;

            /*
             * Avalia a expressao antes de registrar a variavel.
             * Assim, uma inicializacao invalida nao deixa simbolos residuais.
             */
            if (!semantic_evaluate_integer(
                    node->as.variable_declaration.value,
                    table,
                    &value,
                    &division_by_zero))
            {
                if (division_by_zero)
                {
                    fprintf(
                        stderr,
                        "Erro semantico: divisao por zero "
                        "ao inicializar a variavel '%s'.\n",
                        name);
                }
                else
                {
                    fprintf(
                        stderr,
                        "Erro semantico: nao foi possivel inicializar "
                        "a variavel '%s'.\n",
                        name);
                }

                return 0;
            }
        }

        if (!symbol_table_add(table, name, final_type, 0))
        {
            fprintf(
                stderr,
                "Erro semantico: a variavel '%s' ja foi declarada "
                "ou nao foi possivel registra-la.\n",
                name);
            return 0;
        }

        if (final_type == AST_TYPE_INT &&
            !symbol_table_initialize_integer(table, name, value))
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inicializar "
                "a variavel '%s'.\n",
                name);
            return 0;
        }

        return 1;
    }

    
    case AST_CONSTANT_DECLARATION:
    {
        const char *name = node->as.constant_declaration.name;
        ASTType declared_type = node->as.constant_declaration.declared_type;
        ASTType actual_type =
            semantic_infer_type(node->as.constant_declaration.value, table);

        if (declared_type != AST_TYPE_INFERRED &&
            declared_type != actual_type)
        {
            fprintf(
                stderr,
                "Erro semantico: a constante '%s' foi declarada como '%s', "
                "mas recebeu um valor do tipo '%s'.\n",
                name,
                ast_type_name(declared_type),
                ast_type_name(actual_type));
            return 0;
        }

        ASTType final_type =
            declared_type == AST_TYPE_INFERRED
                ? actual_type
                : declared_type;

        if (final_type == AST_TYPE_INFERRED)
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inferir o tipo "
                "da constante '%s'.\n",
                name);
            return 0;
        }

        long value = 0;

        if (final_type == AST_TYPE_INT)
        {
            int division_by_zero = 0;

            /* Valida a inicializacao antes de registrar a constante. */
            if (!semantic_evaluate_integer(
                    node->as.constant_declaration.value,
                    table,
                    &value,
                    &division_by_zero))
            {
                if (division_by_zero)
                {
                    fprintf(
                        stderr,
                        "Erro semantico: divisao por zero "
                        "ao inicializar a constante '%s'.\n",
                        name);
                }
                else
                {
                    fprintf(
                        stderr,
                        "Erro semantico: nao foi possivel inicializar "
                        "a constante '%s'.\n",
                        name);
                }

                return 0;
            }
        }

        if (!symbol_table_add(table, name, final_type, 1))
        {
            fprintf(
                stderr,
                "Erro semantico: a constante '%s' ja foi declarada "
                "ou nao foi possivel registra-la.\n",
                name);
            return 0;
        }

        if (final_type == AST_TYPE_INT &&
            !symbol_table_initialize_integer(table, name, value))
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inicializar "
                "a constante '%s'.\n",
                name);
            return 0;
        }

        return 1;
    }

    case AST_ASSIGNMENT:
    {
        const char *name = node->as.assignment.name;
        const Symbol *symbol = symbol_table_find(table, name);

        if (symbol == NULL)
        {
            fprintf(
                stderr,
                "Erro semantico: a variavel '%s' nao foi declarada.\n",
                name);

            return 0;
        }

        if (symbol->is_constant)
        {
            fprintf(
                stderr,
                "Erro semantico: nao e permitido alterar "
                "a constante '%s'.\n",
                name);

            return 0;
        }

        ASTType actual_type =
            semantic_infer_type(node->as.assignment.value, table);

        if (symbol->type != actual_type)
        {
            fprintf(
                stderr,
                "Erro semantico: a variavel '%s' e do tipo '%s', "
                "mas recebeu um valor do tipo '%s'.\n",
                name,
                ast_type_name(symbol->type),
                ast_type_name(actual_type));

            return 0;
        }

        if (symbol->type == AST_TYPE_INT)
        {
            long value;
            int division_by_zero = 0;

            if (!semantic_evaluate_integer(
                    node->as.assignment.value,
                    table,
                    &value,
                    &division_by_zero))
            {
                if (division_by_zero)
                {
                    fprintf(
                        stderr,
                        "Erro semantico: divisao por zero "
                        "na atribuicao a '%s'.\n",
                        name);
                }
                else
                {
                    fprintf(
                        stderr,
                        "Erro semantico: nao foi possivel avaliar "
                        "o valor atribuido a '%s'.\n",
                        name);
                }

                return 0;
            }

            if (!symbol_table_set_integer(table, name, value))
            {
                fprintf(
                    stderr,
                    "Erro semantico: nao foi possivel atualizar "
                    "a variavel '%s'.\n",
                    name);

                return 0;
            }
        }

        return 1;
    }

    default:
        return 1;
    }
}