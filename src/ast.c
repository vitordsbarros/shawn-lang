#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

/* Cria uma cópia de uma string. */
static char *ast_copy_string(const char *source)
{
    if (source == NULL)
    {
        return NULL;
    }

    size_t length = strlen(source) + 1;
    char *copy = malloc(length);

    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, source, length);
    return copy;
}

/* Aloca e inicializa um nó da AST. */
static ASTNode *ast_new_node(ASTNodeType type)
{
    ASTNode *node = malloc(sizeof(*node));

    if (node == NULL)
    {
        return NULL;
    }

    node->type = type;
    node->inferred_type = AST_TYPE_INFERRED;

    return node;
}

/* Cria um nó que representa um inteiro. */
ASTNode *ast_new_integer(long value)
{
    ASTNode *node = ast_new_node(AST_INTEGER);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.integer_value = value;
    node->inferred_type = AST_TYPE_INT;

    return node;
}

/* Cria um nó que representa um booleano. */
ASTNode *ast_new_boolean(int value)
{
    ASTNode *node = ast_new_node(AST_BOOLEAN);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.boolean_value = value != 0;
    node->inferred_type = AST_TYPE_BOOL;

    return node;
}

/* Cria um nó que representa uma string. */
ASTNode *ast_new_string(const char *value)
{
    if (value == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_STRING);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.string_value = ast_copy_string(value);

    if (node->as.string_value == NULL)
    {
        free(node);
        return NULL;
    }

    node->inferred_type = AST_TYPE_STR;

    return node;
}

/* Cria um nó que representa um identificador. */
ASTNode *ast_new_identifier(const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_IDENTIFIER);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.identifier.name = ast_copy_string(name);

    if (node->as.identifier.name == NULL)
    {
        free(node);
        return NULL;
    }

    return node;
}

/* Cria um nó para uma operação binária. */
ASTNode *ast_new_binary(
    ASTOperator operator,
    ASTNode *left,
    ASTNode *right)
{
    if (left == NULL || right == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_BINARY_EXPR);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.binary.operator = operator;
    node->as.binary.left = left;
    node->as.binary.right = right;

    return node;
}

/* Cria um nó para uma operação unária, como !valor. */
ASTNode *ast_new_unary(
    ASTUnaryOperator operator,
    ASTNode *operand)
{
    if (operand == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_UNARY_EXPR);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.unary.operator = operator;
    node->as.unary.operand = operand;

    return node;
}

/* Cria um nó para uma atribuição. */
ASTNode *ast_new_assignment(const char *name, ASTNode *value)
{
    if (name == NULL || value == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_ASSIGNMENT);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.assignment.name = ast_copy_string(name);

    if (node->as.assignment.name == NULL)
    {
        free(node);
        return NULL;
    }

    node->as.assignment.value = value;

    return node;
}

/* Cria um nó de declaração de variável ou constante. */
static ASTNode *ast_new_declaration(
    ASTNodeType node_type,
    const char *name,
    ASTType declared_type,
    ASTNode *value)
{
    if (name == NULL || value == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(node_type);

    if (node == NULL)
    {
        return NULL;
    }

    char *name_copy = ast_copy_string(name);

    if (name_copy == NULL)
    {
        free(node);
        return NULL;
    }

    if (node_type == AST_VARIABLE_DECLARATION)
    {
        node->as.variable_declaration.name = name_copy;
        node->as.variable_declaration.declared_type = declared_type;
        node->as.variable_declaration.value = value;
    }
    else
    {
        node->as.constant_declaration.name = name_copy;
        node->as.constant_declaration.declared_type = declared_type;
        node->as.constant_declaration.value = value;
    }

    return node;
}

ASTNode *ast_new_variable_declaration(
    const char *name,
    ASTType declared_type,
    ASTNode *value)
{
    return ast_new_declaration(
        AST_VARIABLE_DECLARATION,
        name,
        declared_type,
        value);
}

ASTNode *ast_new_constant_declaration(
    const char *name,
    ASTType declared_type,
    ASTNode *value)
{
    return ast_new_declaration(
        AST_CONSTANT_DECLARATION,
        name,
        declared_type,
        value);
}

/* Cria um bloco vazio. */
ASTNode *ast_new_block(void)
{
    ASTNode *node = ast_new_node(AST_BLOCK);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.block.statements = NULL;
    node->as.block.count = 0;
    node->as.block.capacity = 0;

    return node;
}

/*
 * Adiciona uma instrução ao bloco.
 * A propriedade da instrução passa para o bloco somente em caso de sucesso.
 */
int ast_block_add_statement(ASTNode *block, ASTNode *statement)
{
    if (block == NULL ||
        block->type != AST_BLOCK ||
        statement == NULL)
    {
        return 0;
    }

    if (block->as.block.count >= block->as.block.capacity)
    {
        int new_capacity =
            block->as.block.capacity == 0
                ? 4
                : block->as.block.capacity * 2;

        if (new_capacity < block->as.block.capacity ||
            (size_t)new_capacity >
                (size_t)-1 / sizeof(*block->as.block.statements))
        {
            return 0;
        }

        ASTNode **new_statements = realloc(
            block->as.block.statements,
            (size_t)new_capacity * sizeof(*new_statements));

        if (new_statements == NULL)
        {
            return 0;
        }

        block->as.block.statements = new_statements;
        block->as.block.capacity = new_capacity;
    }

    block->as.block.statements[block->as.block.count] = statement;
    block->as.block.count++;

    return 1;
}

/* Cria um nó condicional com ramos then e else opcionais. */
ASTNode *ast_new_if(
    ASTNode *condition,
    ASTNode *then_branch,
    ASTNode *else_branch)
{
    if (condition == NULL || then_branch == NULL)
    {
        return NULL;
    }

    ASTNode *node = ast_new_node(AST_IF);

    if (node == NULL)
    {
        return NULL;
    }

    node->as.if_statement.condition = condition;
    node->as.if_statement.then_branch = then_branch;
    node->as.if_statement.else_branch = else_branch;

    return node;
}

/* Libera recursivamente uma árvore sintática. */
void ast_free(ASTNode *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
    case AST_INTEGER:
    case AST_BOOLEAN:
        break;

    case AST_STRING:
        free(node->as.string_value);
        break;

    case AST_IDENTIFIER:
        free(node->as.identifier.name);
        break;

    case AST_BINARY_EXPR:
        ast_free(node->as.binary.left);
        ast_free(node->as.binary.right);
        break;

    case AST_UNARY_EXPR:
        ast_free(node->as.unary.operand);
        break;

    case AST_ASSIGNMENT:
        free(node->as.assignment.name);
        ast_free(node->as.assignment.value);
        break;

    case AST_VARIABLE_DECLARATION:
        free(node->as.variable_declaration.name);
        ast_free(node->as.variable_declaration.value);
        break;

    case AST_CONSTANT_DECLARATION:
        free(node->as.constant_declaration.name);
        ast_free(node->as.constant_declaration.value);
        break;

    case AST_BLOCK:
        for (int i = 0; i < node->as.block.count; i++)
        {
            ast_free(node->as.block.statements[i]);
        }

        free(node->as.block.statements);
        break;

    case AST_IF:
        ast_free(node->as.if_statement.condition);
        ast_free(node->as.if_statement.then_branch);
        ast_free(node->as.if_statement.else_branch);
        break;
    }

    free(node);
}

/* Imprime um nó e seus descendentes com indentação. */
static void ast_print_indented(const ASTNode *node, int depth)
{
    if (node == NULL)
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }

    switch (node->type)
    {
    case AST_INTEGER:
        printf("INTEGER: %ld\n", node->as.integer_value);
        break;

    case AST_BOOLEAN:
        printf(
            "BOOLEAN: %s\n",
            node->as.boolean_value ? "true" : "false");
        break;

    case AST_STRING:
        printf("STRING: \"%s\"\n", node->as.string_value);
        break;

    case AST_IDENTIFIER:
        printf("IDENTIFIER: %s\n", node->as.identifier.name);
        break;

    case AST_BINARY_EXPR:
    {
        const char *operator_name;

        switch (node->as.binary.operator)
        {
        case AST_OP_ADD:
            operator_name = "ADD";
            break;
        case AST_OP_SUBTRACT:
            operator_name = "SUBTRACT";
            break;
        case AST_OP_MULTIPLY:
            operator_name = "MULTIPLY";
            break;
        case AST_OP_DIVIDE:
            operator_name = "DIVIDE";
            break;
        case AST_OP_GREATER:
            operator_name = "GREATER";
            break;
        case AST_OP_GREATER_EQUAL:
            operator_name = "GREATER_EQUAL";
            break;
        case AST_OP_LESS:
            operator_name = "LESS";
            break;
        case AST_OP_LESS_EQUAL:
            operator_name = "LESS_EQUAL";
            break;
        case AST_OP_EQUAL:
            operator_name = "EQUAL";
            break;
        case AST_OP_NOT_EQUAL:
            operator_name = "NOT_EQUAL";
            break;
        case AST_OP_LOGICAL_AND:
            operator_name = "LOGICAL_AND";
            break;
        case AST_OP_LOGICAL_OR:
            operator_name = "LOGICAL_OR";
            break;
        default:
            operator_name = "UNKNOWN";
            break;
        }

        printf("BINARY: %s\n", operator_name);
        ast_print_indented(node->as.binary.left, depth + 1);
        ast_print_indented(node->as.binary.right, depth + 1);
        break;
    }

    case AST_UNARY_EXPR:
    {
        const char *operator_name;

        switch (node->as.unary.operator)
        {
        case AST_UNARY_LOGICAL_NOT:
            operator_name = "LOGICAL_NOT";
            break;
        default:
            operator_name = "UNKNOWN";
            break;
        }

        printf("UNARY: %s\n", operator_name);
        ast_print_indented(node->as.unary.operand, depth + 1);
        break;
    }

    case AST_ASSIGNMENT:
        printf("ASSIGNMENT: %s\n", node->as.assignment.name);
        ast_print_indented(node->as.assignment.value, depth + 1);
        break;

    case AST_VARIABLE_DECLARATION:
    {
        ASTType type = node->as.variable_declaration.declared_type;

        if (type == AST_TYPE_INFERRED)
        {
            type = ast_infer_type(node->as.variable_declaration.value);
        }

        printf(
            "VARIABLE DECLARATION: %s (type=%s)\n",
            node->as.variable_declaration.name,
            ast_type_name(type));

        ast_print_indented(node->as.variable_declaration.value, depth + 1);
        break;
    }

    case AST_CONSTANT_DECLARATION:
    {
        ASTType type = node->as.constant_declaration.declared_type;

        if (type == AST_TYPE_INFERRED)
        {
            type = ast_infer_type(node->as.constant_declaration.value);
        }

        printf(
            "CONSTANT DECLARATION: %s (type=%s)\n",
            node->as.constant_declaration.name,
            ast_type_name(type));

        ast_print_indented(node->as.constant_declaration.value, depth + 1);
        break;
    }

    case AST_BLOCK:
        printf("BLOCK (%d statements)\n", node->as.block.count);

        for (int i = 0; i < node->as.block.count; i++)
        {
            ast_print_indented(node->as.block.statements[i], depth + 1);
        }

        break;

    case AST_IF:
        printf("IF\n");

        for (int i = 0; i < depth + 1; i++)
        {
            printf("  ");
        }
        printf("CONDITION:\n");
        ast_print_indented(node->as.if_statement.condition, depth + 2);

        for (int i = 0; i < depth + 1; i++)
        {
            printf("  ");
        }
        printf("THEN:\n");
        ast_print_indented(node->as.if_statement.then_branch, depth + 2);

        if (node->as.if_statement.else_branch != NULL)
        {
            for (int i = 0; i < depth + 1; i++)
            {
                printf("  ");
            }
            printf("ELSE:\n");
            ast_print_indented(node->as.if_statement.else_branch, depth + 2);
        }

        break;
    }
}

/* Ponto de entrada público para imprimir a AST. */
void ast_print(const ASTNode *node)
{
    ast_print_indented(node, 0);
}

const char *ast_type_name(ASTType type)
{
    switch (type)
    {
    case AST_TYPE_INFERRED:
        return "inferred";
    case AST_TYPE_INT:
        return "int";
    case AST_TYPE_FLOAT:
        return "float";
    case AST_TYPE_BOOL:
        return "bool";
    case AST_TYPE_CHAR:
        return "char";
    case AST_TYPE_STR:
        return "str";
    default:
        return "unknown";
    }
}

/* Infere o tipo de um nó sem consultar a tabela de símbolos. */
ASTType ast_infer_type(const ASTNode *node)
{
    if (node == NULL)
    {
        return AST_TYPE_INFERRED;
    }

    switch (node->type)
    {
    case AST_INTEGER:
        return AST_TYPE_INT;

    case AST_BOOLEAN:
        return AST_TYPE_BOOL;

    case AST_STRING:
        return AST_TYPE_STR;

    case AST_UNARY_EXPR:
        if (node->as.unary.operator == AST_UNARY_LOGICAL_NOT &&
            ast_infer_type(node->as.unary.operand) == AST_TYPE_BOOL)
        {
            return AST_TYPE_BOOL;
        }
        return AST_TYPE_INFERRED;

    case AST_BINARY_EXPR:
    {
        ASTType left_type = ast_infer_type(node->as.binary.left);
        ASTType right_type = ast_infer_type(node->as.binary.right);

        switch (node->as.binary.operator)
        {
        case AST_OP_ADD:
        case AST_OP_SUBTRACT:
        case AST_OP_MULTIPLY:
        case AST_OP_DIVIDE:
            if (left_type == AST_TYPE_INT && right_type == AST_TYPE_INT)
            {
                return AST_TYPE_INT;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_GREATER:
        case AST_OP_GREATER_EQUAL:
        case AST_OP_LESS:
        case AST_OP_LESS_EQUAL:
            if (left_type == AST_TYPE_INT && right_type == AST_TYPE_INT)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_EQUAL:
        case AST_OP_NOT_EQUAL:
            if (left_type != AST_TYPE_INFERRED && left_type == right_type)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        case AST_OP_LOGICAL_AND:
        case AST_OP_LOGICAL_OR:
            if (left_type == AST_TYPE_BOOL && right_type == AST_TYPE_BOOL)
            {
                return AST_TYPE_BOOL;
            }
            return AST_TYPE_INFERRED;

        default:
            return AST_TYPE_INFERRED;
        }
    }

    case AST_IDENTIFIER:
    case AST_ASSIGNMENT:
    case AST_VARIABLE_DECLARATION:
    case AST_CONSTANT_DECLARATION:
    case AST_BLOCK:
    case AST_IF:
        return AST_TYPE_INFERRED;
    }

    return AST_TYPE_INFERRED;
}

/* Avalia uma expressão aritmética constante. */
int ast_evaluate_integer(const ASTNode *node, long *result)
{
    if (node == NULL || result == NULL)
    {
        return 0;
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

    if (!ast_evaluate_integer(node->as.binary.left, &left) ||
        !ast_evaluate_integer(node->as.binary.right, &right))
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
            return 0;
        }
        *result = left / right;
        return 1;

    default:
        return 0;
    }
}

/* Avalia expressões booleanas constantes. */
int ast_evaluate_boolean(const ASTNode *node, int *result)
{
    if (node == NULL || result == NULL)
    {
        return 0;
    }

    if (node->type == AST_BOOLEAN)
    {
        *result = node->as.boolean_value;
        return 1;
    }

    if (node->type == AST_UNARY_EXPR)
    {
        if (node->as.unary.operator != AST_UNARY_LOGICAL_NOT)
        {
            return 0;
        }

        int operand;

        if (!ast_evaluate_boolean(node->as.unary.operand, &operand))
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

        if (!ast_evaluate_boolean(node->as.binary.left, &left))
        {
            return 0;
        }

        if (!left)
        {
            *result = 0;
            return 1;
        }

        int right;

        if (!ast_evaluate_boolean(node->as.binary.right, &right))
        {
            return 0;
        }

        *result = right;
        return 1;
    }

    if (operator == AST_OP_LOGICAL_OR)
    {
        int left;

        if (!ast_evaluate_boolean(node->as.binary.left, &left))
        {
            return 0;
        }

        if (left)
        {
            *result = 1;
            return 1;
        }

        int right;

        if (!ast_evaluate_boolean(node->as.binary.right, &right))
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
        long left;
        long right;

        if (!ast_evaluate_integer(node->as.binary.left, &left) ||
            !ast_evaluate_integer(node->as.binary.right, &right))
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

    return 0;
}