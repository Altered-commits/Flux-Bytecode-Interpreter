//This is pretty much a semantic analysis (sort of)
//For each operator, we see if a certain type is compatible or not
//We pick out common stuff to another map

#ifndef UNNAMED_TYPE_CHECKER_HPP
#define UNNAMED_TYPE_CHECKER_HPP

#include <unordered_map>

#include "token.hpp" //For TokenType (operators)
#include "..\Common\common.hpp" //For EvalType

//Easier way ig
#define BINARY_OP_MAPPER(Type1, Type2, EvaluatedType) {std::make_pair(Type1, Type2), EvaluatedType}
#define COMPARISION_OP_MAPPER(Type1, Type2)           {std::make_pair(Type1, Type2), EVAL_INT}

#define ADD_BINARY_OP_CHECKER(Type)      {Type, binOpValidTokens}
#define ADD_COMPARISION_OP_CHECKER(Type) {Type, comparisionAndLogicalValidTokens}

//Hasher for Pair<EvalType, EvalType> using FNV-1a
struct PairHasher {
    std::size_t operator () (const std::pair<EvalType, EvalType>& p) const {
        constexpr uint64_t prime1 = 1099511628211ull;
        constexpr uint64_t prime2 = 1099511628219ull;

        uint64_t hash_value = 14695981039346656037ull; // FNV-1a offset basis

        hash_value ^= static_cast<uint64_t>(p.first);
        hash_value *= prime1;
        hash_value ^= static_cast<uint64_t>(p.second);
        hash_value *= prime2;

        // Mix bits to distribute more evenly
        hash_value ^= hash_value >> 31;
        hash_value *= 0x85ebca6b;
        hash_value ^= hash_value >> 33;

        return static_cast<std::size_t>(hash_value);
    }
};

//Normal Type checker for Operators
//                                                       LeftExpr, RightExpr; EvaluatedType
using TypeEvaluator = const std::unordered_map<std::pair<EvalType, EvalType>, EvalType, PairHasher>;
using TypeChecker   = const std::unordered_map<TokenType, TypeEvaluator>;

//Unary Type checker for Unary Operators
//                                                 UnaryExpr; EvaluatedType
using UnaryTypeEvaluator = const std::unordered_map<EvalType, EvalType>;
using UnaryTypeChecker = const std::unordered_map<TokenType, UnaryTypeEvaluator>;

//--------------TYPE EVALUATOR--------------
//For Binary operations
TypeEvaluator binOpValidTokens = {
    //For Int
    BINARY_OP_MAPPER(EVAL_INT, EVAL_INT, EVAL_INT),
    BINARY_OP_MAPPER(EVAL_INT, EVAL_FLOAT, EVAL_FLOAT),
    //For Float
    BINARY_OP_MAPPER(EVAL_FLOAT, EVAL_INT, EVAL_FLOAT),
    BINARY_OP_MAPPER(EVAL_FLOAT, EVAL_FLOAT, EVAL_FLOAT)
};

//For Comparision and Logical operations
TypeEvaluator comparisionAndLogicalValidTokens = {
    //For Int
    COMPARISION_OP_MAPPER(EVAL_INT, EVAL_INT),
    COMPARISION_OP_MAPPER(EVAL_INT, EVAL_FLOAT),
    //For Float
    COMPARISION_OP_MAPPER(EVAL_FLOAT, EVAL_INT),
    COMPARISION_OP_MAPPER(EVAL_FLOAT, EVAL_FLOAT)
};

//For Unary '!' operations
UnaryTypeEvaluator unaryNotValidTokens = {
    {EVAL_INT,   EVAL_INT},
    {EVAL_FLOAT, EVAL_INT}
};

//For Unary '-' operations
UnaryTypeEvaluator unaryMinusValidTokens = {
    {EVAL_INT,   EVAL_INT},
    {EVAL_FLOAT, EVAL_FLOAT}
};

//--------------TYPE CHECKER--------------
TypeChecker validateTypeForOperator = {
    ADD_BINARY_OP_CHECKER(TOKEN_PLUS),
    ADD_BINARY_OP_CHECKER(TOKEN_MINUS),
    ADD_BINARY_OP_CHECKER(TOKEN_MULT),
    ADD_BINARY_OP_CHECKER(TOKEN_DIV),
    ADD_BINARY_OP_CHECKER(TOKEN_MODULO),
    ADD_BINARY_OP_CHECKER(TOKEN_POW),

    //Comparision / Logical Operator
    ADD_COMPARISION_OP_CHECKER(TOKEN_LT),
    ADD_COMPARISION_OP_CHECKER(TOKEN_GT),
    ADD_COMPARISION_OP_CHECKER(TOKEN_LTEQ),
    ADD_COMPARISION_OP_CHECKER(TOKEN_GTEQ),
    ADD_COMPARISION_OP_CHECKER(TOKEN_EEQ),
    ADD_COMPARISION_OP_CHECKER(TOKEN_NEQ),
    ADD_COMPARISION_OP_CHECKER(TOKEN_AND),
    ADD_COMPARISION_OP_CHECKER(TOKEN_OR)
};

//Additional type checker for Unary Operations
UnaryTypeChecker validateUnaryTypeForOperator = {
    {TOKEN_MINUS, unaryMinusValidTokens},
    {TOKEN_NOT,   unaryNotValidTokens}
};

#endif