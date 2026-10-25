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

        default:
            operator_name = "UNKNOWN";
            break;
        }

        printf("BINARY: %s\n", operator_name);

        ast_print_indented(
            node->as.binary.left,
            depth + 1);

        ast_print_indented(
            node->as.binary.right,
            depth + 1);

        break;
    }

    case AST_ASSIGNMENT:
        printf(
            "ASSIGNMENT: %s\n",
            node->as.assignment.name);

        ast_print_indented(
            node->as.assignment.value,
            depth + 1);

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

        ast_print_indented(
            node->as.variable_declaration.value,
            depth + 1);

        break;
    }

    case AST_CONSTANT_DECLARATION:
        printf(
            "CONSTANT DECLARATION: %s (type=%s)\n",
            node->as.constant_declaration.name,
            ast_type_name(node->as.constant_declaration.declared_type));

        ast_print_indented(
            node->as.constant_declaration.value,
            depth + 1);

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

    case AST_STRING:
        return AST_TYPE_STR;

    case AST_BINARY_EXPR:
    {
        ASTType left_type = ast_infer_type(node->as.binary.left);
        ASTType right_type = ast_infer_type(node->as.binary.right);

        if (left_type == AST_TYPE_INT && right_type == AST_TYPE_INT)
        {
            return AST_TYPE_INT;
        }

        return AST_TYPE_INFERRED;
    }

    default:
        return AST_TYPE_INFERRED;
    }
}

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

    long left;
    long right;

    if (!ast_evaluate_integer(node->as.binary.left, &left) ||
        !ast_evaluate_integer(node->as.binary.right, &right))
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
            return 0;
        }

        *result = left / right;
        return 1;

    default:
        return 0;
    }
}