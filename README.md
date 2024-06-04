# Flux

## Description
Flux is a custom-built bytecode interpreter currently under development. This project is designed to deepen my understanding of interpreters and bytecode execution.

## Supported Features
 - Preprocessor capable of preprocessing `include` and `define` directive without '#' symbol.
 - Compiler capable of:
   - Comparision, Logical and Arithmetic expressions.
   - Variable assignments and re-assignments.
   - If-Elif-Else conditions.
   - For loop (Iterator based).
   - While loop (Condition based).
 - Decently fast Bytecode Interpreter.

## Usage
To compile `.flux` file, use the following command:<br>
```sh
./FluxCompiler [filename].flux
```
This command generates a `Gen.cflx` file.<br>

To interpret, use the following command:<br>
```sh
./FluxInt Gen.cflx
```

#### Note: Interpreter can interpret any compiled flux file with the extension `.cflx`

## Future plans:
1. Functions, maybe even structs.<br>
2. Optimizing Compiler and Interpreter.<br>

**---See the grammar.txt file to get an idea of syntax---**
