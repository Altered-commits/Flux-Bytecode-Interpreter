#ifndef UNNAMED_TOKEN_HPP
#define UNNAMED_TOKEN_HPP

#include <string>

enum TokenType : std::uint8_t {
    //Primitive Types
    TOKEN_INT,
    TOKEN_FLOAT,
    //() Type
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    //{} Type,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    //<> Type
    TOKEN_LT,
    TOKEN_GT,
    //<=, >= Type
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    //Arithmetic Types
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_POW,
    //Assignment Type
    TOKEN_EQ,
    //==, != Type
    TOKEN_EEQ,
    TOKEN_NEQ,
    //&&, !, || Type
    TOKEN_AND,
    TOKEN_NOT,
    TOKEN_OR,
    //Keyword and Identifier Types
    TOKEN_ID,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_CAST,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_IN,
    //, Type
    TOKEN_COMMA,
    //Ternary(? :) Type,
    TOKEN_QUESTION,
    TOKEN_COLON,
    //Statement End
    TOKEN_SEMIC,
    TOKEN_EOF,
    //.. and ... (Range and Ellipsis)
    TOKEN_RANGE,
    TOKEN_ELLIPSIS,
    //Iterator Types
    TOKEN_RANGE_ITER,
    //Idk
    TOKEN_UNKNOWN
};

struct Token
{
    Token(const char* val, TokenType type)
        : token_value(val), token_type(type)
    {}
    Token() = default;

    std::string  token_value;
    TokenType    token_type;
};

#endif