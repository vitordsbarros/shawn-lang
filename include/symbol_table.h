
#ifndef SHAWN_SYMBOL_TABLE_H
#define SHAWN_SYMBOL_TABLE_H

#include "ast.h"

typedef struct
{
    char *name;
    ASTType type;
    int is_constant;
    long integer_value;
    int boolean_value;
} Symbol;

typedef struct SymbolTable
{
    Symbol *symbols;
    int count;
    int capacity;
    struct SymbolTable *parent;
} SymbolTable;

void symbol_table_init(SymbolTable *table);

void symbol_table_init_scope(
    SymbolTable *table,
    SymbolTable *parent);

void symbol_table_free(SymbolTable *table);

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    ASTType type,
    int is_constant);

const Symbol *symbol_table_find(
    const SymbolTable *table,
    const char *name);

const Symbol *symbol_table_find_local(
    const SymbolTable *table,
    const char *name);

int symbol_table_set_integer(
    SymbolTable *table,
    const char *name,
    long value);

int symbol_table_initialize_integer(
    SymbolTable *table,
    const char *name,
    long value);

int symbol_table_set_boolean(
    SymbolTable *table,
    const char *name,
    int value);

int symbol_table_initialize_boolean(
    SymbolTable *table,
    const char *name,
    int value);

#endif