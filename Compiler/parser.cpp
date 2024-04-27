#include <iostream>
#include "parser.hpp"

std::vector<ASTPtr>& Parser::parse()
{
    while(lex.get_current_token().token_type != TOKEN_EOF)
    {
        auto res = parse_statement();
        
        //In case where the input is like: 50 50 instead of: 50 + 50, uk its an error
        if(lex.get_current_token().token_type != TOKEN_SEMIC)
        {
            std::cout << "[ParserError]: Expected tokens: '+', '-', '*','/' or end the statement with a ';'\n";
            std::exit(1);
        }
        advance();
        
        statements.push_back(std::move(res));
    }

    return statements;
}

ASTPtr Parser::parse_statement()
{
    //Variable declaration, float/int identifier = expr;
    if(match_types(TOKEN_KEYWORD_FLOAT) || match_types(TOKEN_KEYWORD_INT))
    {
        //Grab the token type, the type of variable.
        TokenType var_type = current_token.token_type;
        advance();
        
        return parse_variable(var_type, false);
    }
    //When we want identifier = expr;
    if(match_types(TOKEN_ID))
    {
        auto elem = temporary_symbol_table.find(current_token.token_value);
        if(elem == temporary_symbol_table.end())
        {
            std::cout << "[ParserError]: Undefined variable: " << current_token.token_value << '\n';
            std::exit(1);
        }
        return parse_variable(elem->second, true);
    }
    return parse_expr();
}

ASTPtr Parser::parse_variable(TokenType var_type, bool is_reassignment)
{
    //Grab the identifier
    if(!match_types(TOKEN_ID))
    {
        std::cout << "[ParserError]: Expected identifer after type.\n";
        std::exit(1);
    }

    //Variable name should not clash with the existing names
    std::string identifier = current_token.token_value;
    if((temporary_symbol_table.find(identifier) != temporary_symbol_table.end()) && (!is_reassignment))
    {
        std::cout << "[ParserError]: Variable name already exists, use another one.\n";
        std::exit(1);
    }
    advance();

    //Check for Equals symbol
    if(!match_types(TOKEN_EQ))
    {
        std::cout << "[ParserError]: Expected '=' token after identifier.\n";
        std::exit(1);
    }
    advance();

    //The value of variable, the expression
    auto var_expr  = parse_expr();
    auto expr_type = var_expr->evaluateExprType();

    if((expr_type == TOKEN_FLOAT && var_type == TOKEN_KEYWORD_FLOAT)
        ||
        (expr_type == TOKEN_INT && var_type == TOKEN_KEYWORD_INT))
    {
        //Add it to symbol table and create AST
        temporary_symbol_table[identifier] = var_type;
        return create_variable_assign_node(var_type, identifier, std::move(var_expr));
    }
    //Oops, types dont match, errrorrrrr!
    else
    {
        std::cout << "[ParserError]: Evaluated expression type doesn't match the type specified to variable.\n";
        std::exit(1);
    }
}

//common binary operations such as addition, subtraction etc
ASTPtr Parser::common_binary_op(
        ParseFuncPtr callback_left,
        TokenType first,
        TokenType second,
        ParseFuncPtr callback_right = nullptr)
{
    if(callback_right == nullptr)
        callback_right = callback_left;
    
    auto left = callback_left();

    
    while (match_types(first) || match_types(second))
    {
        //save the current operator type
        TokenType op = current_token.token_type;
        advance();

        auto right = callback_right();
        
        left = create_binary_op_node(op, std::move(left), std::move(right));
    }
    
    return left;
}

//Plus/Minus operation for two numbers
ASTPtr Parser::parse_expr()
{
    return common_binary_op(std::bind(&parse_term, this), TOKEN_PLUS, TOKEN_MINUS);
}

//Multiply/Divide operation for two numbers
ASTPtr Parser::parse_term()
{
    return common_binary_op(std::bind(&parse_factor, this), TOKEN_MULT, TOKEN_DIV);
}

//Unary Op or Power function
ASTPtr Parser::parse_factor()
{
    switch (current_token.token_type)
    {
        //Unary ops (-5, +30, etc)
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        {
            TokenType op_type = current_token.token_type;

            advance();
            auto expr = parse_factor();

            return create_unary_op_node(op_type, std::move(expr));
        }
    }

    return parse_power();
}

//For power operations, like 2 ^ 2 etc
ASTPtr Parser::parse_power()
{
    return common_binary_op(std::bind(&parse_atom, this), TOKEN_POW, TOKEN_POW, std::bind(&parse_factor, this));
}

//Primitive elements: INT, FLOAT, (), Cast<>() etc
ASTPtr Parser::parse_atom()
{
    switch (current_token.token_type)
    {
        //Variable getter
        case TOKEN_ID:
        {
            auto elem = temporary_symbol_table.find(current_token.token_value);
            if(elem != temporary_symbol_table.end())
            {
                advance();
                return create_variable_access_node(elem->second, elem->first);
            }
            else
            {
                std::cout << "[ParserError]: Undefined variable: " << current_token.token_value << '\n';
                std::exit(1);
            }
        }
        //Values
        case TOKEN_INT:
        case TOKEN_FLOAT:
        {
            Token saved_token = current_token;
            advance();
            return create_value_node(saved_token);
        }
        //( Expression )
        case TOKEN_LPAREN:
        {
            advance();
            auto expr = parse_expr();

            if(!match_types(TOKEN_RPAREN))
            {
                std::cerr << "[ParserError]: Expected ')'\n";
                std::exit(1);
            }

            advance();
            return expr;
        }
        //Cast<Type>() aka casting to a specific type
        case TOKEN_KEYWORD_CAST:
            return parse_cast();
        //Welp
        default:
        {
            std::cout << "[ParserError]: Expected some sort of Integer or Float / Unary expression.\n";
            std::exit(1);
        }
    }
}

ASTPtr Parser::parse_cast()
{
    advance();
    //Look for '<'
    if(!match_types(TOKEN_LT))
    {
        std::cout << "[ParserError]: Expected '<'.\n";
        std::exit(1);
    }
    advance();
    
    //Get a type: Int, Float, etc
    if((!match_types(TOKEN_KEYWORD_INT)) && (!match_types(TOKEN_KEYWORD_FLOAT)))
    {
        std::cout << "[ParserError]: Expected a type for 'Cast' keyword: like Int, Float, etc.\n";
        std::exit(1);
    }
    TokenType eval_type = current_token.token_type;
    advance();

    //Look for '>'
    if(!match_types(TOKEN_GT))
    {
        std::cout << "[ParserError]: Expected '>'.\n";
        std::exit(1);
    }
    advance();

    //look for '(' before parsing expr
    if(!match_types(TOKEN_LPAREN))
    {
        std::cout << "[ParserError]: Expected '('.\n";
        std::exit(1);
    }
    advance();

    auto expr = parse_expr();

    //look for ')'
    if(!match_types(TOKEN_RPAREN))
    {
        std::cout << "[ParserError]: Expected ')'.\n";
        std::exit(1);
    }
    advance();

    //construct and return the dummy ast node
    return create_cast_dummy_node(eval_type, std::move(expr));
}

//------------------------ Node creation ------------------------
ASTPtr Parser::create_value_node(const Token& token)
{
    return std::make_unique<ASTValue>(token.token_type, token.token_value);
}

ASTPtr Parser::create_binary_op_node(TokenType op_type, ASTPtr&& left, ASTPtr&& right)
{
    return std::make_unique<ASTBinaryOp>(op_type, std::move(left), std::move(right));
}

ASTPtr Parser::create_unary_op_node(TokenType op_type, ASTPtr&& expr)
{
    return std::make_unique<ASTUnaryOp>(op_type, std::move(expr));
}

ASTPtr Parser::create_variable_assign_node(TokenType var_type, const std::string& identifier, ASTPtr&& var_expr)
{
    return std::make_unique<ASTVariableAssign>(identifier, var_type, std::move(var_expr));
}

ASTPtr Parser::create_variable_access_node(TokenType var_type, const std::string& identifier)
{
    return std::make_unique<ASTVariableAccess>(identifier, var_type);
}

ASTPtr Parser::create_cast_dummy_node(TokenType eval_type, ASTPtr&& expr)
{
    return std::make_unique<ASTCastDummy>(eval_type, std::move(expr));
}

//------------------------ Helper functions ------------------------
void Parser::advance()
{
    current_token = lex.get_token();
}

//can be used for error handling later
bool Parser::match_types(TokenType type)
{
    return current_token.token_type == type;
}