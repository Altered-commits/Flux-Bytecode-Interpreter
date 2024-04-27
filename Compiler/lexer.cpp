#include <iostream>
#include "lexer.hpp"

void Lexer::advance()
{
    cur_chr = text[++cur_pos];
}

char Lexer::peek(std::uint8_t offset)
{
    if(offset + cur_pos < text_length)
        return text[offset + cur_pos];
    return text[text_length - 1];
}

void Lexer::skip_spaces()
{
    while (SANITY_CHECK(cur_chr == ' ' || cur_chr == '\t' || cur_chr == '\n'))
        advance();
}

void Lexer::lex_digits()
{
    std::string  temp{};
    std::uint8_t dot_count{0};

    while (SANITY_CHECK(IS_DIGIT(cur_chr) || cur_chr == '.'))
    {
        if(cur_chr == '.')
            dot_count += 1;
        
        if(dot_count > 1)
        {
            std::cout << "[LexerError]: Multiple '.' found while tokenizing float!";
            std::exit(1);
        }

        temp += cur_chr;
        
        advance();
    }

    dot_count == 0 ? set_token(temp, TOKEN_INT)
                   : set_token(temp, TOKEN_FLOAT);
}

void Lexer::lex_identifier_or_keyword()
{
    std::string temp{};

    while(SANITY_CHECK(IS_IDENT(cur_chr)))
    {
        temp += cur_chr;
        advance();
    }

    //Using the identifier map, if the string exists in the map, its a keyword, else its just identifier
    auto elem = identifier_map.find(temp);
    if(elem != identifier_map.end())
        set_token(temp, elem->second);
    else
        set_token(temp, TOKEN_ID);
}

void Lexer::lex() 
{
    skip_spaces();

    if(IS_DIGIT(cur_chr))
    {
        lex_digits();
        return;
    }
    if(IS_CHAR(cur_chr))
    {
        lex_identifier_or_keyword();
        return;
    }

    switch (cur_chr)
    {
        case '+':
            set_token("+", TOKEN_PLUS);
            advance();
            return;
        case '-':
            set_token("-", TOKEN_MINUS);
            advance();
            return;
        case '*':
            set_token("*", TOKEN_MULT);
            advance();
            return;
        case '/':
            set_token("/", TOKEN_DIV);
            advance();
            return;
        case '(':
            set_token("(", TOKEN_LPAREN);
            advance();
            return;
        case ')':
            set_token(")", TOKEN_RPAREN);
            advance();
            return;
        case '<':
            set_token("<", TOKEN_LT);
            advance();
            return;
        case '>':
            set_token(">", TOKEN_GT);
            advance();
            return;
        case '^':
            set_token("^", TOKEN_POW);
            advance();
            return;
        case '=':
            set_token("=", TOKEN_EQ);
            advance();
            return;
        case ';':
            set_token(";", TOKEN_SEMIC);
            advance();
            return;
        case '\0':
            set_token("EOF", TOKEN_EOF);
            return;
        default:
            std::cout << "[LexerError]: Character not supported: " << (int)cur_chr << '\n';
            std::exit(1);
    }

}

Token& Lexer::get_token()
{
    lex();
    return token;
}

Token& Lexer::get_current_token()
{
    return token;
}

void Lexer::set_token(const std::string& token_text, TokenType token_type)
{
    token.token_type  = token_type;
    token.token_value = token_text;
}