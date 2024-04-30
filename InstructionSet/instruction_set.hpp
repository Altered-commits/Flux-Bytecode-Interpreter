#ifndef UNNAMED_INST_SET
#define UNNAMED_INST_SET

#include <cstdint>

enum ILInstruction : std::uint8_t
{
    PUSH_INT,
    PUSH_FLOAT,
    //Integer instructions
    ADD,
    SUB,
    MUL,
    DIV,
    //Floating point version of normal integer instruction
    FADD,
    FSUB,
    FMUL,
    FDIV,
    //Unary instructions (NEGATION, etc)
    NEG,
    //Power instruction
    POW,
    //Variable instructions
    ASSIGN_VAR,
    ASSIGN_VAR_NO_POP,
    ACCESS_VAR,
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
    //EOF
    END_OF_FILE
};

#endif