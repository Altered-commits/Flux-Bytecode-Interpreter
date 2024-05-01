#include <iostream>
#include "lexer.hpp"

//Global stuff ig
std::size_t Lexer::line = 1;
std::size_t Lexer::col = 1;

void Lexer::advance()
{
    if(cur_chr == '\n') {
        ++line; col = 0;
    }
    else
        ++col;
    cur_chr = text[++cur_pos];
}

char Lexer::peek(std::uint8_t offset)
{
    return (offset + cur_pos < text_length) ? text[offset + cur_pos] : text[text_length - 1];
}

void Lexer::skip_spaces()
{
    while (SANITY_CHECK(cur_chr == ' ' || cur_chr == '\t' || cur_chr == '\n'))
        advance();
}

void Lexer::skip_single_line_comments()
{
    while (SANITY_CHECK(cur_chr != '\n'))
        advance();
    //skip the newline as well
    advance();
}
/**/
void Lexer::skip_multi_line_comments()
{
    while (SANITY_CHECK(!(cur_chr == '*' && peek(1) == '/')))
        advance();

    //skip '*' and '/'
    advance();
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

        temp += cur_chr;
        
        advance();
    }

    if(dot_count > 1)
        printError("LexerError", "Floating number can only have at max one '.', multiple '.' found");

    set_token(temp, (dot_count == 0) ? TOKEN_INT : TOKEN_FLOAT);
}

void Lexer::lex_identifier_or_keyword()
{
    std::string temp{};

    while(SANITY_CHECK(IS_IDENT(cur_chr)))
    {
        temp.push_back(cur_chr);
        advance();
    }

    //Using the identifier map, if the string exists in the map, its a keyword, else its just identifier
    auto elem = identifier_map.find(temp);
    set_token(temp, (elem != identifier_map.end()) ? elem->second : TOKEN_ID);
}

void Lexer::lex_this_or_eq_variation(const char* text, const char* text_with_eq, TokenType type, TokenType type_with_eq)
{
    advance();
    if(cur_chr == '=')
    {
        advance();
        set_token(text_with_eq, type_with_eq);
        return;
    }
    set_token(text, type);
    return;
}

void Lexer::lex() 
{
    while(true)
    {
        skip_spaces();

        if(IS_DIGIT(cur_chr))
        {
            lex_digits();
            return;
        }
        if(IS_CHAR(cur_chr) || cur_chr == '_')
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
            //Check for comments as well
            case '/':
                advance();
                //Single line comments
                if(cur_chr == '/')
                {
                    skip_single_line_comments();
                    break;
                }
                //Multi line comments
                if(cur_chr == '*')
                {
                    skip_multi_line_comments();
                    break;
                }
                set_token("/", TOKEN_DIV);
                return;
            case '(':
                set_token("(", TOKEN_LPAREN);
                advance();
                return;
            case ')':
                set_token(")", TOKEN_RPAREN);
                advance();
                return;
            case '^':
                set_token("^", TOKEN_POW);
                advance();
                return;
            case '&':
                advance();
                if(cur_chr == '&') {
                    advance();
                    set_token("&&", TOKEN_AND);
                    return;
                }
                printError("LexerError", "'&' bitwise operator currently not supported");
                return;
            case '|':
                advance();
                if(cur_chr == '|') {
                    advance();
                    set_token("||", TOKEN_OR);
                    return;
                }
                printError("LexerError", "'|' bitwise operator currently not supported");
                return;
            //Functions self explanatory
            case '<':
                //Check either '<' or '<='
                lex_this_or_eq_variation("<", "<=", TOKEN_LT, TOKEN_LTEQ);
                return;
            case '>':
                //Check either '>' or '>='
                lex_this_or_eq_variation(">", ">=", TOKEN_GT, TOKEN_GTEQ);
                return;
            case '!':
                //Either '!' not operator, or '!=' operator
                lex_this_or_eq_variation("!", "!=", TOKEN_NOT, TOKEN_NEQ);
                return;
            case '=':
                //Check if its '==' or simple '='
                lex_this_or_eq_variation("=", "==", TOKEN_EQ, TOKEN_EEQ);
                return;
            //,
            case ',':
                set_token(",", TOKEN_COMMA);
                advance();
                return;
            //End of statements
            case ';':
                set_token(";", TOKEN_SEMIC);
                advance();
                return;
            case '\0':
                set_token("EOF", TOKEN_EOF);
                return;
            
            default:
                printError("LexerError", "Character not supported, Character: ", cur_chr);
        }
    }
}
//------------------------------------------
Token& Lexer::get_token()
{
    lex();
    return token;
}

Token& Lexer::get_current_token()
{
    return token;
}

Token Lexer::peek_next_token()
{
    //Save lexer state
    Token saved_token = token;
    std::uint64_t saved_pos = cur_pos;
    char saved_char = cur_chr;
    
    //Lex
    lex();
    Token next = token;
    
    //Rewind lexer state
    token = saved_token;
    cur_pos = saved_pos;
    cur_chr = saved_char;
    
    //Return
    return next;
}

void Lexer::set_token(const std::string& token_text, TokenType token_type)
{
    token.token_type  = token_type;
    token.token_value = token_text;
}

std::pair<std::size_t, std::size_t> Lexer::getLineColCount()
{
    return {line, col};
}