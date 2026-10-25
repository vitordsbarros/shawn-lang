#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <arquivo.sn>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");

    if (file == NULL)
    {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        perror("Erro ao ler o arquivo");
        fclose(file);
        return 1;
    }

    long file_size = ftell(file);

    if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        perror("Erro ao determinar o tamanho do arquivo");
        fclose(file);
        return 1;
    }

    char *source = malloc((size_t)file_size + 1);

    if (source == NULL)
    {
        fprintf(stderr, "Erro: memoria insuficiente.\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(source, 1, (size_t)file_size, file);

    if (bytes_read != (size_t)file_size || ferror(file))
    {
        fprintf(stderr, "Erro ao ler o arquivo.\n");
        free(source);
        fclose(file);
        return 1;
    }

    source[bytes_read] = '\0';
    fclose(file);

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    SymbolTable table;
    symbol_table_init(&table);

    while (parser.current_token.type != TOKEN_EOF)
    {
        ASTNode *statement = parser_parse_statement(&parser);

        if (statement == NULL)
        {
            fprintf(stderr, "Erro de sintaxe ao analisar o programa.\n");
            symbol_table_free(&table);
            free(source);
            return 1;
        }

        ast_print(statement);

        if (!semantic_analyze(statement, &table))
        {
            ast_free(statement);
            symbol_table_free(&table);
            free(source);
            return 1;
        }

        ast_free(statement);
    }

    for (int i = 0; i < table.count; i++)
    {
        if (table.symbols[i].type == AST_TYPE_INT)
        {
            printf(
                "Valor final de %s: %ld\n",
                table.symbols[i].name,
                table.symbols[i].integer_value);
        }
    }

    symbol_table_free(&table);
    free(source);

    return 0;
}