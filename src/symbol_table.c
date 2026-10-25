
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

void symbol_table_init_scope(SymbolTable *table, SymbolTable *parent)
{
    if (table == NULL)
    {
        return;
    }

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    table->parent = parent;
}

void symbol_table_init(SymbolTable *table)
{
    symbol_table_init_scope(table, NULL);
}

void symbol_table_free(SymbolTable *table)
{
    if (table == NULL)
    {
        return;
    }

    for (int i = 0; i < table->count; i++)
    {
        free(table->symbols[i].name);
    }

    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    table->parent = NULL;
}

const Symbol *symbol_table_find_local(
    const SymbolTable *table,
    const char *name)
{
    if (table == NULL || name == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < table->count; i++)
    {
        if (strcmp(table->symbols[i].name, name) == 0)
        {
            return &table->symbols[i];
        }
    }

    return NULL;
}

const Symbol *symbol_table_find(
    const SymbolTable *table,
    const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }

    for (const SymbolTable *scope = table;
         scope != NULL;
         scope = scope->parent)
    {
        const Symbol *symbol = symbol_table_find_local(scope, name);

        if (symbol != NULL)
        {
            return symbol;
        }
    }

    return NULL;
}

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    ASTType type,
    int is_constant)
{
    if (table == NULL || name == NULL || type == AST_TYPE_INFERRED)
    {
        return 0;
    }

    /* Impede duplicatas apenas no escopo atual.
       Uma variável local pode ocultar uma variável externa. */
    if (symbol_table_find_local(table, name) != NULL)
    {
        return 0;
    }

    if (table->count == table->capacity)
    {
        int new_capacity = table->capacity == 0
                              ? 4
                              : table->capacity * 2;

        Symbol *new_symbols = realloc(
            table->symbols,
            (size_t)new_capacity * sizeof(*new_symbols));

        if (new_symbols == NULL)
        {
            return 0;
        }

        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }

    size_t length = strlen(name) + 1;
    char *name_copy = malloc(length);

    if (name_copy == NULL)
    {
        return 0;
    }

    memcpy(name_copy, name, length);

    Symbol symbol;
    symbol.name = name_copy;
    symbol.type = type;
    symbol.is_constant = is_constant;
    symbol.integer_value = 0;
    symbol.boolean_value = 0;

    table->symbols[table->count] = symbol;
    table->count++;

    return 1;
}

static Symbol *symbol_table_find_mutable(
    SymbolTable *table,
    const char *name)
{
    if (table == NULL || name == NULL)
    {
        return NULL;
    }

    for (SymbolTable *scope = table;
         scope != NULL;
         scope = scope->parent)
    {
        for (int i = 0; i < scope->count; i++)
        {
            if (strcmp(scope->symbols[i].name, name) == 0)
            {
                return &scope->symbols[i];
            }
        }
    }

    return NULL;
}

int symbol_table_set_integer(
    SymbolTable *table,
    const char *name,
    long value)
{
    Symbol *symbol = symbol_table_find_mutable(table, name);

    if (symbol == NULL ||
        symbol->type != AST_TYPE_INT ||
        symbol->is_constant)
    {
        return 0;
    }

    symbol->integer_value = value;
    return 1;
}

int symbol_table_initialize_integer(
    SymbolTable *table,
    const char *name,
    long value)
{
    Symbol *symbol = symbol_table_find_mutable(table, name);

    if (symbol == NULL || symbol->type != AST_TYPE_INT)
    {
        return 0;
    }

    symbol->integer_value = value;
    return 1;
}

int symbol_table_set_boolean(
    SymbolTable *table,
    const char *name,
    int value)
{
    Symbol *symbol = symbol_table_find_mutable(table, name);

    if (symbol == NULL ||
        symbol->type != AST_TYPE_BOOL ||
        symbol->is_constant)
    {
        return 0;
    }

    symbol->boolean_value = value != 0;
    return 1;
}

int symbol_table_initialize_boolean(
    SymbolTable *table,
    const char *name,
    int value)
{
    Symbol *symbol = symbol_table_find_mutable(table, name);

    if (symbol == NULL || symbol->type != AST_TYPE_BOOL)
    {
        return 0;
    }

    symbol->boolean_value = value != 0;
    return 1;
}