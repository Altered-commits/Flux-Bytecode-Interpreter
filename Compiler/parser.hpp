#ifndef UNNAMED_PARSER_HPP
#define UNNAMED_PARSER_HPP

#include <functional>
#include <map>

#include "lexer.hpp"
#include "ast.hpp"

using ParseFuncPtr = std::function<ASTPtr(void)>;

class Parser
{
    public:
        Parser(const std::string& text) 
            : lex(text), current_token(lex.get_token())
        {}

        std::vector<ASTPtr>& parse();
        
    private:
        ASTPtr parse_statement();
        ASTPtr parse_expr();
        ASTPtr parse_term();
        ASTPtr parse_factor();
        ASTPtr parse_power();
        ASTPtr parse_atom();
        ASTPtr parse_variable(TokenType, bool);
        ASTPtr parse_cast();
        ASTPtr common_binary_op(ParseFuncPtr, TokenType, TokenType, ParseFuncPtr);
    
    private:
        ASTPtr create_value_node(const Token&);
        ASTPtr create_binary_op_node(TokenType, ASTPtr&&, ASTPtr&&);
        ASTPtr create_unary_op_node(TokenType, ASTPtr&&);
        ASTPtr create_variable_assign_node(TokenType, const std::string&, ASTPtr&&);
        ASTPtr create_variable_access_node(TokenType, const std::string&);
        ASTPtr create_cast_dummy_node(TokenType, ASTPtr&&);

    private:
        void advance();
        bool match_types(TokenType);

    private:
        Lexer lex;
        Token& current_token;

        std::vector<ASTPtr> statements;
        //Temporary symbol table for variables
        std::map<std::string, TokenType> temporary_symbol_table;
};

#endif