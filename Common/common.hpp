//Stuff common for both Compiler and Interpreter
#ifndef UNNAMED_INST_SET
#define UNNAMED_INST_SET

#include <cstdint>
#include <cstring>
#include <variant>

//----------------------EVALUATED TYPES----------------------
//A tag telling what the expression is going to evaluate to, either an Integer, Float, etc
enum EvalType : std::uint8_t
{
    EVAL_INT,
    EVAL_FLOAT,

    //Expr evaluated to an iterator types
    EVAL_RANGE_ITER,

    //For unknown types
    EVAL_UNKNOWN
};

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
    //Arithmetic instructions
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    //Unary instructions (NEGATION, etc)
    NEG,
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
    DESTROY_MULTIPLE_SYMBOL_TABLES, //Multiple levels
    //EOF
    END_OF_FILE
};

//----------------------INSTRUCTION----------------------
using InstructionValue = std::variant<int, float, std::uint16_t, std::size_t, std::string>;
struct Instruction
{
    InstructionValue value;
    ILInstruction    inst;

    //Additional data, for scope indexing (for instructions like ACCESS_VAR or ASSIGN_VAR)
    //Idk why im having this as a literal 16bit value but ight
    std::uint16_t scopeIndexIfNeeded;

    Instruction(ILInstruction inst, InstructionValue&& value = {}, std::uint16_t scopeIndex = 0)
        : inst(inst), value(std::move(value)), scopeIndexIfNeeded(scopeIndex)
    {}
};


//----------------------File extension checker----------------------
static bool checkFileExt(const char* const EXT, const char* filename)
{
    size_t len    = strlen(filename);
    size_t extLen = strlen(EXT);
    if (len < extLen || strcmp(filename + len - extLen, EXT) != 0)
        return false;
    
    return true;
}

#endif