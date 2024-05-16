//Stuff common for both Compiler and Interpreter
#ifndef UNNAMED_INST_SET
#define UNNAMED_INST_SET

#include <cstdint>

//----------------------ITERATORS TYPES----------------------
enum IteratorType : std::uint8_t
{
    RANGE_ITERATOR
};

//----------------------INSTRUCTIONS TYPES----------------------
enum ILInstruction : std::uint8_t
{
    PUSH_INT,
    PUSH_FLOAT,
    //Integer instructions
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    //Floating point version of normal integer instruction
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FMOD,
    //Unary instructions (NEGATION, etc)
    NEG,
    //Power instruction
    POW,
    //Variable instructions
    ASSIGN_VAR,
    ASSIGN_VAR_NO_POP,
    //Having to add alot more instructions cuz im a bad coder
    REASSIGN_VAR,
    REASSIGN_VAR_NO_POP,
    ACCESS_VAR,
    DATAINST_VAR_SCOPE_IDX,
    //Casting instruction
    CAST_INT,
    CAST_FLOAT,
    //Comparision instructions
    CMP_EQ,
    CMP_NEQ,
    CMP_GT,
    CMP_LT,
    CMP_GTEQ,
    CMP_LTEQ,
    //Logical operations
    AND,
    OR,
    NOT,
    //Jump operations
    JUMP_IF_FALSE,
    JUMP,
    //Iteration operations
    ITER_INIT,
    ITER_HAS_NEXT,
    ITER_NEXT,
    ITER_CURRENT,
    ITER_RECALC_STEP,
    DATAINST_ITER_ID,
    //Scope symbol table related operation
    CREATE_SYMBOL_TABLE,
    DESTROY_SYMBOL_TABLE,
    DESTROY_MULTIPLE_SYM_TABLES, //Multiple levels
    //EOF
    END_OF_FILE
};
#endif