#include <iostream>
#include "parser.hpp"

std::vector<ASTPtr>& Parser::parse()
{
    while(lex.get_current_token().token_type != TOKEN_EOF)
    {
        auto res = parse_statement();
        
        //In case where the input is like: 50 50 instead of: 50 + 50, uk its an error
        if(lex.get_current_token().token_type != TOKEN_SEMIC)
            printError("ParserError", "Expected tokens: '+', '-', '*','/' or end the statement with a ';'");
    
        advance();
        
        statements.push_back(std::move(res));
    }

    return statements;
}

//-----------------TECHNICALLY Helper Functions-----------------
ASTPtr Parser::parse_cast()
{
    advance();
    //Look for '<'
    if(!match_types(TOKEN_LT))
        printError("ParserError", "'Cast' Expected '<'");
        
    advance();
    
    //Look for a type 
    switch (current_token.token_type)
    {
        //Perfect
        case TOKEN_KEYWORD_INT:
        case TOKEN_KEYWORD_FLOAT:
            break;
   
        default:
            printError("ParserError", "'Cast' Expected a type after '<', like Int, Float, Etc");
    }
    
    TokenType eval_type = current_token.token_type;
    advance();

    //Look for '>'
    if(!match_types(TOKEN_GT))
        printError("ParserError", "'Cast' Expected '>'");

    advance();

    //look for '(' before parsing expr
    if(!match_types(TOKEN_LPAREN))
        printError("ParserError", "'Cast' Expected '('");

    advance();

    auto expr = parse_expr();

    //look for ')'
    if(!match_types(TOKEN_RPAREN))
        printError("ParserError", "'Cast' Expected ')'");

    advance();

    //construct and return the dummy ast node
    return create_cast_dummy_node(eval_type, std::move(expr));
}

ASTPtr Parser::parse_variable(TokenType var_type, bool is_reassignment)
{
    //Grab the identifier
    if(!match_types(TOKEN_ID))
        printError("ParserError", "Expected identifier after type");

    //Variable name should not clash with the existing names
    std::string identifier = current_token.token_value;
    if((temporary_symbol_table.find(identifier) != temporary_symbol_table.end()) && (!is_reassignment))
        printError("ParserError", "Current variable: ", identifier, " already exists, use another name");

    advance();

    //Check for Equals symbol
    if(!match_types(TOKEN_EQ))
        printError("ParserError", "Expected '=' after identifier");

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
    printError("ParserError", "Evaluated expression type doesn't match the type specified to variable.");    
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

//Overload of common_binary_op for multiple params
ASTPtr Parser::common_binary_op(
    ParseFuncPtr callback_left,
    std::size_t mask,
    ParseFuncPtr callback_right = nullptr)
{
    if(callback_right == nullptr)
        callback_right = callback_left;
    
    auto left = callback_left();

    while(SAME_PRECEDENCE_MASK_CHECK(current_token.token_type, mask))
    {
        //save the current operator type
        TokenType op = current_token.token_type;
        advance();

        auto right = callback_right();
        
        left = create_binary_op_node(op, std::move(left), std::move(right));
    }

    return left;
}

//-----------------START OF PARSING-----------------
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

    return parse_expr();
}

//Plus/Minus operation for two numbers, variable re-assignment
ASTPtr Parser::parse_expr()
{
    //When we want identifier = expr; re-assigning variables
    if(match_types(TOKEN_ID))
    {
        if(peek().token_type == TOKEN_EQ)
        {
            auto elem = temporary_symbol_table.find(current_token.token_value);
            if(elem == temporary_symbol_table.end())
                printError("ParserError", "Undefined variable: ", current_token.token_value);

            return parse_variable(elem->second, true);
        }
    }

    return common_binary_op(std::bind(parse_comp_expr, this), TOKEN_AND, TOKEN_OR, std::bind(parse_comp_expr, this));
}

//! expression or arith_expr along with  
ASTPtr Parser::parse_comp_expr()
{
    //For ! expr
    if(match_types(TOKEN_NOT))
    {
        TokenType type = current_token.token_type;
        advance();
        
        //Get Comparision expression again
        auto expr = parse_comp_expr();

        return create_unary_op_node(type, std::move(expr));
    }
    //For the other stuff, they all have same precedance so just pass them all in common binary op
    ///Note: Mask (comparisionTypeMask) already exists in ast.hpp
    return common_binary_op(std::bind(parse_arith_expr, this), comparisionTypeMask, std::bind(parse_arith_expr, this));
}

//Plus/Minus operation
ASTPtr Parser::parse_arith_expr()
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
                printError("ParserError", "Undefined variable: ", current_token.token_value);
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
                printError("ParserError", "Expected ')'");

            advance();
            return expr;
        }
        //Cast<Type>() aka casting to a specific type
        case TOKEN_KEYWORD_CAST:
            return parse_cast();
        //Welp
        default:
            printError("ParserError", "Expected some sort of Integer or Float / Unary expression");
    }
}

//-----------------NODE CREATION-----------------
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

//-----------------ACTUALLY Helper functions-----------------
void Parser::advance()
{
    current_token = lex.get_token();
}

Token Parser::peek()
{
    return lex.peek_next_token();
}

//can be used for error handling later
bool Parser::match_types(TokenType type)
{
    return current_token.token_type == type;
}