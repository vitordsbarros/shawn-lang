# Changelog

All notable changes to Shawn are documented in this file.

Shawn is an experimental interpreted programming language written in C. The project is in early development, and syntax, semantics, and runtime behavior may change as development progresses.

## [Unreleased]

### Planned
- Expand support for data types and runtime behavior.
- Improve syntax, semantic, and runtime error reporting.
- Expand language documentation and examples.
- Continue improving automated test coverage.
- Develop additional language features, including functions.

## [0.3.0] - 2026-10-25

### Added
- Conditional statements using `if` and `else`.
- Support for nested conditional statements.
- Boolean literals: `true` and `false`.
- Relational operators: `>`, `>=`, `<`, and `<=`.
- Equality operators: `==` and `!=`.
- Logical operators: `&&`, `||`, and `!`.
- Short-circuit evaluation for logical AND (`&&`) and OR (`||`).
- Lexical scopes for blocks.
- Variable shadowing within nested scopes.
- Automated tests for conditional statements and lexical scopes.

### Improved
- Expression parsing and operator precedence.
- Semantic validation of expressions and conditional conditions.
- Scope-aware variable lookup and assignment.
- Validation of conditional branches, including branches that are not executed.
- Test coverage across parser behavior, AST evaluation, semantic analysis, and symbol table operations.

### Notes
- This is an early development milestone and a pre-release.
- Shawn now supports conditional control flow and lexical scoping.
- Full language and runtime integration remains under development.
- Functions and broader data type support remain planned.

## [0.2.0] - 2026-10-24

### Added
- Integer assignments with arithmetic expressions and variable references.
- Semantic type checking for assignments.
- Division-by-zero detection during variable initialization, constant initialization, and assignment.
- Expanded automated semantic tests.

### Improved
- Semantic analysis for variable and constant declarations.
- Constant protection against reassignment.
- Error handling to prevent invalid variable declarations from being registered in the symbol table.

### Notes
- This is an early development milestone and a pre-release.
- Broader type support, control-flow statements, functions, and additional runtime capabilities remained future work at this stage.
- Syntax, semantics, and runtime behavior may change as development progresses.

## [0.1.0] - 2026-10-22

### Added
- Initial C project structure.
- Lexer and token definitions.
- Parser for supported statements and arithmetic expressions.
- Abstract Syntax Tree (AST) structures and memory management.
- Integer arithmetic expression evaluation.
- Basic semantic checks for variable and constant declarations.
- Symbol table with integer value storage.
- Duplicate declaration checks.
- Protection against modifying constants through the symbol table API.
- Makefile targets for building the interpreter and running tests.
- Initial automated tests for parsing, symbol table behavior, and integer expression evaluation.

### Notes
- This was the first development milestone and a pre-release.
- Full assignment execution, complete runtime integration, and broader type support were not yet finished.
- This release established the initial foundation for Shawn's ongoing development.