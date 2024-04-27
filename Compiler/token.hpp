#ifndef UNNAMED_TOKEN_HPP
#define UNNAMED_TOKEN_HPP

#include <string>
#include <map>

enum TokenType {
    //Primitive Types
    TOKEN_INT,
    TOKEN_FLOAT,
    //() Type
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    //<> Type
    TOKEN_LT,
    TOKEN_GT,
    //Arithmetic Types
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_POW,
    //Assignment Type
    TOKEN_EQ,
    //Keyword and Identifier Types
    TOKEN_ID,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_CAST,
    //Statement End
    TOKEN_SEMIC,
    TOKEN_EOF
};

struct Token
{
    std::string  token_value;
    TokenType    token_type;
};

#endif