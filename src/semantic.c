
#include <stdio.h>
#include <stdlib.h>

#include "semantic.h"

static void semantic_free_cloned_symbol_table(SymbolTable *table);

static int semantic_clone_symbol_table(
    const SymbolTable *source,
    SymbolTable *destination)
{
    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    SymbolTable *parent_copy = NULL;

    if (source->parent != NULL)
    {
        parent_copy = malloc(sizeof(*parent_copy));

        if (parent_copy == NULL)
        {
            return 0;
        }

        if (!semantic_clone_symbol_table(source->parent, parent_copy))
        {
            free(parent_copy);
            return 0;
        }
    }

    symbol_table_init_scope(destination, parent_copy);

    for (int i = 0; i < source->count; i++)
    {
        const Symbol *original = &source->symbols[i];

        if (!symbol_table_add(
                destination,
                original->name,
                original->type,
                original->is_constant))
        {
            semantic_free_cloned_symbol_table(destination);
            return 0;
        }

        int initialized = 0;

        if (original->type == AST_TYPE_INT)
        {
            initialized = symbol_table_initialize_integer(
                destination,
                original->name,
                original->integer_value);
        }
        else if (original->type == AST_TYPE_BOOL)
        {
            initialized = symbol_table_initialize_boolean(
                destination,
                original->name,
                original->boolean_value);
        }

        if (!initialized)
        {
            semantic_free_cloned_symbol_table(destination);
            return 0;
        }
    }

    return 1;
}

static void semantic_free_cloned_symbol_table(SymbolTable *table)
{
    if (table == NULL)
    {
        return;
    }

    SymbolTable *parent = table->parent;
    table->parent = NULL;

    symbol_table_free(table);

    if (parent != NULL)
    {
        semantic_free_cloned_symbol_table(parent);
        free(parent);
    }
}

/* Infere o tipo considerando os identificadores da tabela de simbolos. */
static ASTType semantic_infer_type(
    const ASTNode *node,
    const SymbolTable *table)
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

        return symbol != NULL ? symbol->type : AST_TYPE_INFERRED;
    }

    case AST_UNARY_EXPR:
    {
        ASTType operand_type =
            semantic_infer_type(node->as.unary.operand, table);

        if (node->as.unary.operator == AST_UNARY_LOGICAL_NOT &&
            operand_type == AST_TYPE_BOOL)
        {
            return AST_TYPE_BOOL;
        }

        return AST_TYPE_INFERRED;
    }

    case AST_BINARY_EXPR:
    {
        ASTOperator operator = node->as.binary.operator;

        ASTType left_type =
            semantic_infer_type(node->as.binary.left, table);
        ASTType right_type =
            semantic_infer_type(node->as.binary.right, table);

        switch (operator)
        {
        case AST_OP_ADD:
        case AST_OP_SUBTRACT:
        case AST_OP_MULTIPLY:
        case AST_OP_DIVIDE:
            if (left_type == AST_TYPE_INT &&
                right_type == AST_TYPE_INT)
            {
                return AST_TYPE_INT;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_GREATER:
        case AST_OP_GREATER_EQUAL:
        case AST_OP_LESS:
        case AST_OP_LESS_EQUAL:
            if (left_type == AST_TYPE_INT &&
                right_type == AST_TYPE_INT)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_EQUAL:
        case AST_OP_NOT_EQUAL:
            if (left_type != AST_TYPE_INFERRED &&
                left_type == right_type)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_LOGICAL_AND:
        case AST_OP_LOGICAL_OR:
            if (left_type == AST_TYPE_BOOL &&
                right_type == AST_TYPE_BOOL)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        default:
            return AST_TYPE_INFERRED;
        }
    }

    default:
        return ast_infer_type(node);
    }
}

/* Avalia expressoes inteiras, inclusive aquelas com identificadores. */
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

    ASTOperator operator = node->as.binary.operator;

    if (operator != AST_OP_ADD &&
        operator != AST_OP_SUBTRACT &&
        operator != AST_OP_MULTIPLY &&
        operator != AST_OP_DIVIDE)
    {
        return 0;
    }

    long left;
    long right;

    if (!semantic_evaluate_integer(
            node->as.binary.left, table, &left, division_by_zero) ||
        !semantic_evaluate_integer(
            node->as.binary.right, table, &right, division_by_zero))
    {
        return 0;
    }

    switch (operator)
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

/* Avalia expressoes booleanas, incluindo comparacoes e identificadores. */
static int semantic_evaluate_boolean(
    const ASTNode *node,
    const SymbolTable *table,
    int *result,
    int *division_by_zero)
{
    if (node == NULL || table == NULL || result == NULL)
    {
        return 0;
    }

    if (node->type == AST_BOOLEAN)
    {
        *result = node->as.boolean_value;
        return 1;
    }

    if (node->type == AST_IDENTIFIER)
    {
        const Symbol *symbol =
            symbol_table_find(table, node->as.identifier.name);

        if (symbol == NULL || symbol->type != AST_TYPE_BOOL)
        {
            return 0;
        }

        *result = symbol->boolean_value;
        return 1;
    }

    if (node->type == AST_UNARY_EXPR)
    {
        if (node->as.unary.operator != AST_UNARY_LOGICAL_NOT)
        {
            return 0;
        }

        int operand;

        if (!semantic_evaluate_boolean(
                node->as.unary.operand,
                table,
                &operand,
                division_by_zero))
        {
            return 0;
        }

        *result = !operand;
        return 1;
    }

    if (node->type != AST_BINARY_EXPR)
    {
        return 0;
    }

    ASTOperator operator = node->as.binary.operator;

    if (operator == AST_OP_LOGICAL_AND)
    {
        int left;

        if (!semantic_evaluate_boolean(
                node->as.binary.left,
                table,
                &left,
                division_by_zero))
        {
            return 0;
        }

        if (!left)
        {
            *result = 0;
            return 1;
        }

        int right;

        if (!semantic_evaluate_boolean(
                node->as.binary.right,
                table,
                &right,
                division_by_zero))
        {
            return 0;
        }

        *result = right;
        return 1;
    }

    if (operator == AST_OP_LOGICAL_OR)
    {
        int left;

        if (!semantic_evaluate_boolean(
                node->as.binary.left,
                table,
                &left,
                division_by_zero))
        {
            return 0;
        }

        if (left)
        {
            *result = 1;
            return 1;
        }

        int right;

        if (!semantic_evaluate_boolean(
                node->as.binary.right,
                table,
                &right,
                division_by_zero))
        {
            return 0;
        }

        *result = right;
        return 1;
    }

    if (operator == AST_OP_GREATER ||
        operator == AST_OP_GREATER_EQUAL ||
        operator == AST_OP_LESS ||
        operator == AST_OP_LESS_EQUAL ||
        operator == AST_OP_EQUAL ||
        operator == AST_OP_NOT_EQUAL)
    {
        ASTType left_type =
            semantic_infer_type(node->as.binary.left, table);
        ASTType right_type =
            semantic_infer_type(node->as.binary.right, table);

        if (left_type != right_type ||
            left_type == AST_TYPE_INFERRED)
        {
            return 0;
        }

        if (left_type == AST_TYPE_INT)
        {
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

            switch (operator)
            {
            case AST_OP_GREATER:
                *result = left > right;
                return 1;

            case AST_OP_GREATER_EQUAL:
                *result = left >= right;
                return 1;

            case AST_OP_LESS:
                *result = left < right;
                return 1;

            case AST_OP_LESS_EQUAL:
                *result = left <= right;
                return 1;

            case AST_OP_EQUAL:
                *result = left == right;
                return 1;

            case AST_OP_NOT_EQUAL:
                *result = left != right;
                return 1;

            default:
                return 0;
            }
        }

        if (left_type == AST_TYPE_BOOL &&
            (operator == AST_OP_EQUAL ||
             operator == AST_OP_NOT_EQUAL))
        {
            int left;
            int right;

            if (!semantic_evaluate_boolean(
                    node->as.binary.left,
                    table,
                    &left,
                    division_by_zero) ||
                !semantic_evaluate_boolean(
                    node->as.binary.right,
                    table,
                    &right,
                    division_by_zero))
            {
                return 0;
            }

            *result = operator == AST_OP_EQUAL
                          ? left == right
                          : left != right;
            return 1;
        }

        return 0;
    }

    return 0;
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
    case AST_CONSTANT_DECLARATION:
    {
        int is_constant = node->type == AST_CONSTANT_DECLARATION;

        const char *name = is_constant
                               ? node->as.constant_declaration.name
                               : node->as.variable_declaration.name;

        ASTType declared_type = is_constant
                                    ? node->as.constant_declaration.declared_type
                                    : node->as.variable_declaration.declared_type;

        ASTNode *value_node = is_constant
                                  ? node->as.constant_declaration.value
                                  : node->as.variable_declaration.value;

        ASTType actual_type = semantic_infer_type(value_node, table);

        if (declared_type != AST_TYPE_INFERRED &&
            declared_type != actual_type)
        {
            fprintf(
                stderr,
                "Erro semantico: %s '%s' foi declarada como '%s', "
                "mas recebeu um valor do tipo '%s'.\n",
                is_constant ? "a constante" : "a variavel",
                name,
                ast_type_name(declared_type),
                ast_type_name(actual_type));
            return 0;
        }

        ASTType final_type = declared_type == AST_TYPE_INFERRED
                                 ? actual_type
                                 : declared_type;

        if (final_type == AST_TYPE_INFERRED)
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inferir o tipo "
                "%s '%s'.\n",
                is_constant ? "da constante" : "da variavel",
                name);
            return 0;
        }

        long integer_value = 0;
        int boolean_value = 0;
        int division_by_zero = 0;

        if (final_type == AST_TYPE_INT)
        {
            if (!semantic_evaluate_integer(
                    value_node, table, &integer_value, &division_by_zero))
            {
                if (division_by_zero)
                {
                    fprintf(
                        stderr,
                        "Erro semantico: divisao por zero ao inicializar "
                        "%s '%s'.\n",
                        is_constant ? "a constante" : "a variavel",
                        name);
                }
                else
                {
                    fprintf(
                        stderr,
                        "Erro semantico: nao foi possivel inicializar "
                        "%s '%s'.\n",
                        is_constant ? "a constante" : "a variavel",
                        name);
                }
                return 0;
            }
        }
        else if (final_type == AST_TYPE_BOOL)
        {
            if (!semantic_evaluate_boolean(
                    value_node, table, &boolean_value, &division_by_zero))
            {
                fprintf(
                    stderr,
                    "Erro semantico: nao foi possivel avaliar o valor "
                    "booleano de %s '%s'.\n",
                    is_constant ? "a constante" : "a variavel",
                    name);
                return 0;
            }
        }
        else
        {
            fprintf(
                stderr,
                "Erro semantico: inicializacao para o tipo '%s' "
                "ainda nao e suportada.\n",
                ast_type_name(final_type));
            return 0;
        }

        if (!symbol_table_add(table, name, final_type, is_constant))
        {
            fprintf(
                stderr,
                "Erro semantico: %s '%s' ja foi declarada "
                "ou nao foi possivel registra-la.\n",
                is_constant ? "a constante" : "a variavel",
                name);
            return 0;
        }

        int initialized = final_type == AST_TYPE_INT
                              ? symbol_table_initialize_integer(
                                    table, name, integer_value)
                              : symbol_table_initialize_boolean(
                                    table, name, boolean_value);

        if (!initialized)
        {
            fprintf(
                stderr,
                "Erro semantico: nao foi possivel inicializar "
                "%s '%s'.\n",
                is_constant ? "a constante" : "a variavel",
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
                fprintf(
                    stderr,
                    division_by_zero
                        ? "Erro semantico: divisao por zero na atribuicao a '%s'.\n"
                        : "Erro semantico: nao foi possivel avaliar o valor atribuido a '%s'.\n",
                    name);
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
        else if (symbol->type == AST_TYPE_BOOL)
        {
            int value;
            int division_by_zero = 0;

            if (!semantic_evaluate_boolean(
                    node->as.assignment.value,
                    table,
                    &value,
                    &division_by_zero))
            {
                fprintf(
                    stderr,
                    "Erro semantico: nao foi possivel avaliar o valor "
                    "booleano atribuido a '%s'.\n",
                    name);
                return 0;
            }

            if (!symbol_table_set_boolean(table, name, value))
            {
                fprintf(
                    stderr,
                    "Erro semantico: nao foi possivel atualizar "
                    "a variavel '%s'.\n",
                    name);
                return 0;
            }
        }
        else
        {
            fprintf(
                stderr,
                "Erro semantico: atribuicao para o tipo '%s' "
                "ainda nao e suportada.\n",
                ast_type_name(symbol->type));
            return 0;
        }

        return 1;
    }

    case AST_INTEGER:
    case AST_BOOLEAN:
    case AST_STRING:
    case AST_IDENTIFIER:
    case AST_BINARY_EXPR:
    case AST_UNARY_EXPR:
    {
        ASTType type = semantic_infer_type(node, table);

        if (type == AST_TYPE_INFERRED)
        {
            fprintf(
                stderr,
                "Erro semantico: expressao com tipos invalidos "
                "ou impossiveis de inferir.\n");
            return 0;
        }

        if (type == AST_TYPE_INT)
        {
            long value;
            int division_by_zero = 0;

            if (!semantic_evaluate_integer(
                    node, table, &value, &division_by_zero))
            {
                fprintf(
                    stderr,
                    division_by_zero
                        ? "Erro semantico: divisao por zero na expressao.\n"
                        : "Erro semantico: nao foi possivel avaliar "
                          "a expressao inteira.\n");
                return 0;
            }
        }
        else if (type == AST_TYPE_BOOL)
        {
            int value;
            int division_by_zero = 0;

            if (!semantic_evaluate_boolean(
                    node, table, &value, &division_by_zero))
            {
                fprintf(
                    stderr,
                    "Erro semantico: expressao booleana invalida.\n");
                return 0;
            }
        }

        return 1;
    }

    case AST_BLOCK:
    {
        SymbolTable block_scope;
        symbol_table_init_scope(&block_scope, table);

        int success = 1;

        for (int i = 0; i < node->as.block.count; i++)
        {
            if (!semantic_analyze(
                    node->as.block.statements[i],
                    &block_scope))
            {
                success = 0;
                break;
            }
        }

        symbol_table_free(&block_scope);
        return success;
    }

    case AST_IF:
    {
        ASTNode *condition = node->as.if_statement.condition;
        ASTNode *then_branch = node->as.if_statement.then_branch;
        ASTNode *else_branch = node->as.if_statement.else_branch;

        ASTType condition_type = semantic_infer_type(condition, table);

        if (condition_type != AST_TYPE_BOOL)
        {
            fprintf(
                stderr,
                "Erro semantico: a condicao do 'if' deve ser do tipo 'bool'.\n");
            return 0;
        }

        int condition_value = 0;
        int division_by_zero = 0;

        if (!semantic_evaluate_boolean(
                condition,
                table,
                &condition_value,
                &division_by_zero))
        {
            fprintf(
                stderr,
                division_by_zero
                    ? "Erro semantico: divisao por zero na condicao do 'if'.\n"
                    : "Erro semantico: nao foi possivel avaliar a condicao do 'if'.\n");
            return 0;
        }

        /*
         * Valida cada ramo em uma copia independente e em um
         * escopo filho. As alteracoes da validacao sao descartadas.
         */
        ASTNode *branches[] = {then_branch, else_branch};

        for (int i = 0; i < 2; i++)
        {
            if (branches[i] == NULL)
            {
                continue;
            }

            SymbolTable validation_table;

            if (!semantic_clone_symbol_table(table, &validation_table))
            {
                fprintf(
                    stderr,
                    "Erro semantico: nao foi possivel preparar "
                    "a validacao do 'if'.\n");
                return 0;
            }

            SymbolTable branch_scope;
            symbol_table_init_scope(&branch_scope, &validation_table);

            int valid = semantic_analyze(
                branches[i],
                &branch_scope);

            symbol_table_free(&branch_scope);
            semantic_free_cloned_symbol_table(&validation_table);

            if (!valid)
            {
                return 0;
            }
        }

        /*
         * Executa somente o ramo selecionado em um escopo filho
         * da tabela real. Declaracoes do ramo nao escapam dele.
         */
        ASTNode *selected_branch = condition_value
                                       ? then_branch
                                       : else_branch;

        if (selected_branch == NULL)
        {
            return 1;
        }

        SymbolTable branch_scope;
        symbol_table_init_scope(&branch_scope, table);

        int success = semantic_analyze(
            selected_branch,
            &branch_scope);

        symbol_table_free(&branch_scope);

        return success;
    }

    default:
        fprintf(
            stderr,
            "Erro semantico: tipo de no da AST nao suportado.\n");
        return 0;
    }
}