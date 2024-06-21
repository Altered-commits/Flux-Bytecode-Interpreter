//Stuff common for both Compiler and Interpreter
#ifndef UNNAMED_INST_SET
#define UNNAMED_INST_SET

#include <cstdint>
#include <cstring>
#include <variant>
#include <vector>

//----------------------EVALUATED TYPES----------------------
//A tag telling what the expression is going to evaluate to, either an Integer, Float, etc
enum EvalType : std::uint8_t
{
    EVAL_AUTO, //Accept any type, deduced by compiler
    EVAL_INT,
    EVAL_FLOAT,
    EVAL_VOID,

    //Expr evaluated to an iterator types
    EVAL_RANGE_ITER,

    //Callable type: Function
    EVAL_CALLABLE,

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
    PUSH_INT64,
    PUSH_UINT64,
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
    CMP_IS,
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
    //Functions, Return
    FUNC_START,
    FUNC_CALL,
    FUNC_END,
    RETURN,
    USE_RETURN_VAL,
    //EOF
    END_OF_FILE
};

//----------------------INSTRUCTION----------------------
using InstructionValue = std::variant<std::int64_t, std::uint64_t, double, std::uint16_t, std::string>;
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
using ListOfInstruction = std::vector<Instruction>;

//----------------------File extension checker----------------------
static bool checkFileExt(const char* const EXT, const char* filename)
{
    size_t len    = strlen(filename);
    size_t extLen = strlen(EXT);
    if (len < extLen || strcmp(filename + len - extLen, EXT) != 0)
        return false;
    
    return true;
}

//PURELY FOR DEBUGGING PURPOSES
static const char* const ILInstructionToString(ILInstruction inst) {
    // Static array for fast conversion
    static constexpr const char* const ILInstructionStrings[] = {
        "PUSH_INT64",
        "PUSH_UINT64",
        "PUSH_FLOAT",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "POW",
        "NEG",
        "ASSIGN_VAR",
        "ASSIGN_VAR_NO_POP",
        "REASSIGN_VAR",
        "REASSIGN_VAR_NO_POP",
        "ACCESS_VAR",
        "DATAINST_VAR_SCOPE_IDX",
        "CAST_INT",
        "CAST_FLOAT",
        "CMP_EQ",
        "CMP_NEQ",
        "CMP_GT",
        "CMP_LT",
        "CMP_GTEQ",
        "CMP_LTEQ",
        "AND",
        "OR",
        "NOT",
        "JUMP_IF_FALSE",
        "JUMP",
        "ITER_INIT",
        "ITER_HAS_NEXT",
        "ITER_NEXT",
        "ITER_CURRENT",
        "ITER_RECALC_STEP",
        "DATAINST_ITER_ID",
        "CREATE_SYMBOL_TABLE",
        "DESTROY_SYMBOL_TABLE",
        "DESTROY_MULTIPLE_SYMBOL_TABLES",
        "FUNC_START",
        "FUNC_CALL",
        "FUNC_END",
        "RETURN",
        "USE_RETURN_VAL",
        "END_OF_FILE"
    };

    // Convert enum value to string
    return ILInstructionStrings[static_cast<std::size_t>(inst)];
}
#endif