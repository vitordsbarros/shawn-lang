# Shawn

**An experimental interpreted programming language written in C.**

Shawn is an experimental programming language built from scratch in C. Its goal is to develop an interpreter with its own syntax, lexer, parser, abstract syntax tree (AST), semantic analysis, and symbol table.

> **Status: Early development — v0.3.0 (pre-release).** Shawn is under active development and is not ready for production use. Syntax, semantics, and runtime behavior may change as the interpreter evolves.

## Features

### Language Features
- Variable declarations and assignments.
- Integer (`int`) and boolean (`bool`) types.
- Arithmetic expressions with operator precedence.
- Relational operators: `>`, `>=`, `<`, and `<=`.
- Equality operators: `==` and `!=`.
- Logical operators: `&&`, `||`, and `!`.
- Boolean literals: `true` and `false`.
- Conditional statements using `if` and `else`.
- Nested conditional statements.
- Block-based lexical scopes.
- Variable shadowing in nested scopes.
- Strict type checking without implicit conversions.
- Constant protection against reassignment.

### Interpreter Components
- Lexer and tokenization.
- Recursive-descent parser.
- Abstract Syntax Tree (AST) construction, evaluation, and memory management.
- Semantic analysis for declarations, assignments, expressions, and conditional branches.
- Scope-aware symbol table with nested scopes.
- Short-circuit evaluation for logical AND (`&&`) and OR (`||`).
- Error handling for invalid expressions, incompatible types, undeclared variables, and division by zero.
- Automated tests for parsing, AST evaluation, semantic analysis, conditionals, and lexical scopes.

## Example

The following program declares two integer variables and uses a conditional to assign a value based on the user's age.

```shawn
set int: idade = 20;
set int: acesso = 0;

if (idade >= 18) {
    acesso = 1;
} else {
    acesso = 0;
}
```

### Boolean Expressions

Shawn supports boolean literals and logical operators.

```shawn
set bool: autorizado = true;
set bool: bloqueado = false;

if (autorizado && !bloqueado) {
    set int: resultado = 1;
} else {
    set int: resultado = 0;
}
```

### Lexical Scopes

Blocks create lexical scopes. Variables declared inside a block are local to that scope, while assignments to visible outer variables update the existing variable.

```shawn
set int: total = 10;

if (true) {
    set int: parcial = 5;
    total = total + parcial;
}
```

These examples illustrate the current language syntax. The interpreter remains under development, and complete runtime integration is still a work in progress.

## Requirements

- GCC with C11 support.
- MSYS2/MinGW on Windows, or a compatible C development environment.
- `mingw32-make` on the current Windows setup, or `make` on systems where it is available.

## Installation and Build

Clone the repository:

```bash
git clone https://github.com/vitordsbarros/shawn-lang.git
cd shawn-lang
```

Build the project:

```bash
mingw32-make all
```

The build target produces `build/shawn`.

## Running Tests

Run the automated test suite:

```bash
mingw32-make test
```

The tests cover:

- Parser behavior and expression precedence.
- Arithmetic and boolean AST evaluation.
- Logical operators and short-circuit behavior.
- Variable declarations, assignments, and type checking.
- Constant reassignment protection.
- Conditional statements and nested conditionals.
- Lexical scopes and variable shadowing.
- Error handling for invalid expressions and assignments.

Remove generated build artifacts:

```bash
mingw32-make clean
```

## Project Structure

```text
shawnLang/
├── include/    # Header files
├── src/        # Interpreter components
├── tests/      # Automated test suite
├── examples/   # Example Shawn programs
├── build/      # Generated build artifacts
├── main.c      # Program entry point
├── Makefile    # Build and test targets
├── LICENSE
├── README.md
└── CHANGELOG.md
```

## Roadmap

- [ ] Complete integration of parsing, semantic analysis, and program execution.
- [ ] Expand support for data types and runtime behavior.
- [ ] Improve syntax, semantic, and runtime error reporting.
- [ ] Develop additional language features, including functions.
- [ ] Expand language documentation and examples.
- [ ] Continue improving automated test coverage.

## Version History

| Version | Milestone |
|---|---|
| `v0.1.0` | Initial lexer, parser, AST, and integer expression evaluation |
| `v0.2.0` | Improved semantic analysis and assignment execution |
| `v0.3.0` | Conditional control flow, logical expressions, and lexical scopes |

See [CHANGELOG.md](CHANGELOG.md) for the detailed history of changes.

## Contributing

Shawn is an experimental project under active development. Bug reports, suggestions, and contributions are welcome.

Please open an issue before making substantial changes so the proposed work can be discussed first.

## License

Shawn is distributed under the MIT License. See [LICENSE](LICENSE) for details.