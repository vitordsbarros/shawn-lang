
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude

BUILD_DIR = build

COMMON_SRC = src/ast.c src/lexer.c src/parser.c src/semantic.c src/symbol_table.c

TESTS = parser_test symbol_table_test ast_evaluate_test semantic_test if_test scope_test

.PHONY: all test clean

all: $(BUILD_DIR)/shawn

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/shawn: $(COMMON_SRC) main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) main.c -o $@

$(BUILD_DIR)/parser_test: $(COMMON_SRC) tests/parser_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) tests/parser_test.c -o $@

$(BUILD_DIR)/symbol_table_test: $(COMMON_SRC) tests/symbol_table_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) tests/symbol_table_test.c -o $@

$(BUILD_DIR)/ast_evaluate_test: src/ast.c tests/ast_evaluate_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) src/ast.c tests/ast_evaluate_test.c -o $@

$(BUILD_DIR)/semantic_test: $(COMMON_SRC) tests/semantic_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) tests/semantic_test.c -o $@

$(BUILD_DIR)/if_test: $(COMMON_SRC) tests/if_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) tests/if_test.c -o $@

$(BUILD_DIR)/scope_test: $(COMMON_SRC) tests/scope_test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_SRC) tests/scope_test.c -o $@

test: $(addprefix $(BUILD_DIR)/,$(TESTS))
	./$(BUILD_DIR)/parser_test
	./$(BUILD_DIR)/symbol_table_test
	./$(BUILD_DIR)/ast_evaluate_test
	./$(BUILD_DIR)/semantic_test
	./$(BUILD_DIR)/if_test
	./$(BUILD_DIR)/scope_test

clean:
	rm -rf $(BUILD_DIR)