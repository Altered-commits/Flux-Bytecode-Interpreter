#include <iostream>
#include "parser.hpp"

std::vector<ASTPtr>& Parser::parse()
{
    while(lex.get_current_token().token_type != TOKEN_EOF)
    {
        auto res = parse_statement();
        
        statements.push_back(std::move(res));
    }

    return statements;
}

//-----------------TECHNICALLY Helper Functions-----------------
ASTPtr Parser::parse_if_condition()
{
    // Check '('
    if(!match_types(TOKEN_LPAREN))
        printError("ParserError", "Expected '(' after 'If'");
    
    advance();

    //Now parse condition
    auto if_condition = parse_expr();

    //Now we need ')'
    if(!match_types(TOKEN_RPAREN))
        printError("ParserError", "Expected ')' after condition for If clause");
    advance();

    //Look for '{' body '}'
    ASTPtr if_body = parse_block();

    //Now check for Elif clauses, if they exist that is. Condition, Body
    std::vector<std::pair<ASTPtr, ASTPtr>> elif_clauses;
    while(match_types(TOKEN_KEYWORD_ELIF))
    {
        advance();
        //same thing, look for '( condition ')
        if(!match_types(TOKEN_LPAREN)) printError("ParserError", "Expected '(' after 'Elif'");
        advance();
        
        auto elif_condition = parse_expr();
        
        if(!match_types(TOKEN_RPAREN)) printError("ParserError", "Expected ')' for Elif clause after condition");
        advance();

        //'{' body '}'
        auto elif_body = parse_block();

        elif_clauses.push_back({std::move(elif_condition), std::move(elif_body)});
    }

    //Look for else if it exists
    ASTPtr else_block = nullptr;
    if(match_types(TOKEN_KEYWORD_ELSE))
    {
        advance();
        //Just look for a code block for Else
        else_block = parse_block();
    }

    //With all the information create IfNode
    return create_if_node(std::move(if_condition), std::move(if_body), std::move(elif_clauses), std::move(else_block));
}

ASTPtr Parser::parse_block()
{
    if(!match_types(TOKEN_LBRACE)) printError("ParserError", "Expected '{' after ')'");
    advance();

    std::vector<ASTPtr> statements;
    while(!match_types(TOKEN_RBRACE))
    {
        auto statement = parse_statement();
        //Add to list of statements
        statements.emplace_back(std::move(statement));
    }
    
    if(!match_types(TOKEN_RBRACE)) printError("ParserError", "Expected '}'");
    advance();

    return create_block_node(std::move(statements));
}

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

ASTPtr Parser::parse_reassignment(TokenType var_type)
{
    //Grab the identifier
    if(!match_types(TOKEN_ID))
        printError("ParserError", "Expected identifier after type");

    //Variable name should exist
    std::string identifier = current_token.token_value;
    if(get_variable_from_symbol_table(identifier) == TOKEN_UNKNOWN)
        printError("ParserError", "Current variable: '", identifier, "' doesn't exist.");

    advance();

    //Check for Equals symbol
    if(!match_types(TOKEN_EQ))
        printError("ParserError", "Variable reassignment requires '=' after identifier");
    
    advance();

    //The value of variable, the expression
    auto var_expr  = parse_expr();
    auto expr_type = var_expr->evaluateExprType();

    if(keyword_to_primitive_type.at(var_type) == expr_type)
    {
        //Add it to symbol table and create AST
        add_variable_to_symbol_table(identifier, var_type);
        return create_variable_assign_node(var_type, identifier, std::move(var_expr), true);
    }
    //Oops, types dont match, errrorrrrr!
    printError("ParserError", "Evaluated expression type doesn't match the type pre-assigned to variable: ", identifier);
}

ASTPtr Parser::parse_declaration(TokenType var_type)
{
    std::vector<ASTPtr> declarations;
    //We check for multiple variables to assign for
    //Type identifier, identifier = expr, identifier;
    while(true)
    {
        //Grab the identifier
        if(!match_types(TOKEN_ID))
            printError("ParserError", "Expected identifier after type");

        //Variable name should not clash with the existing names
        std::string identifier = current_token.token_value;
        if(find_variable_from_current_scope(identifier))
            printError("ParserError", "Current variable: ", identifier, " already exists, use another name");

        advance();

        //Check for Equals symbol, if not found, we assuming its like Int a; initialize the value to 0
        if(!match_types(TOKEN_EQ))
        {
            add_variable_to_symbol_table(identifier, var_type);
            //Yeah its a bit hard to understand but uhh yeah
            //Bro the compiler is useless, even tho i declared last arg to default to false, it still expects me to put value
            declarations.emplace_back(create_variable_assign_node(
                    var_type, identifier, create_value_node(Token("0", keyword_to_primitive_type.at(var_type))), false));
        }
        //We found '=' symbol
        else
        {
            advance();

            //The value of variable, the expression
            auto var_expr  = parse_expr();
            auto expr_type = var_expr->evaluateExprType();

            if(keyword_to_primitive_type.at(var_type) == expr_type)
            {
                //Add it to symbol table and create AST
                add_variable_to_symbol_table(identifier, var_type);
                declarations.emplace_back(create_variable_assign_node(var_type, identifier, std::move(var_expr), false));
            }
            else
                //Oops, types dont match, errrorrrrr!
                printError("ParserError", "Evaluated expression type doesn't match the type assigned to variable: ", identifier);
        }
        //Now we look for ',' if we dont find it, then we know that we reached the end of it;
        if(!match_types(TOKEN_COMMA))
            break;
        advance();
    }

    //return ASTBlock after we r done parsing this
    return create_block_node(std::move(declarations));
}

ASTPtr Parser::parse_variable(TokenType var_type, bool is_reassignment)
{
    switch (is_reassignment)
    {
        case true:
            return parse_reassignment(var_type);
        case false:
            return parse_declaration(var_type);
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
    ASTPtr    function_return_value = nullptr;
    TokenType statement_type = current_token.token_type;

    switch(statement_type)
    {
        case TOKEN_KEYWORD_FLOAT:
        case TOKEN_KEYWORD_INT:
        {
            TokenType var_type = current_token.token_type;
            advance();
            function_return_value = parse_variable(var_type, false);
        }
        break;
        case TOKEN_KEYWORD_IF:
        {
            advance();
            create_scope();
            function_return_value = parse_if_condition();
            destroy_scope();
        }
        break;
        default:
            function_return_value = parse_expr();
            break;
    }
    
    //Semi colon only when some conditions aka ignore some keywords
    if(!(statement_type >= TOKEN_KEYWORD_IF
        && 
        statement_type <= TOKEN_KEYWORD_ELSE))
    {
        //If the expression makes sense, the user should also end it with a semicolon as well
        if(!match_types(TOKEN_SEMIC))
            printError("ParserError", "Expected tokens: '+', '-', '*','/' or end the statement with a ';'");

        advance();
    }

    return function_return_value;
}

//Plus/Minus operation for two numbers, variable re-assignment
ASTPtr Parser::parse_expr()
{
    //When we want identifier = expr; re-assigning variables
    if(match_types(TOKEN_ID))
    {
        if(peek().token_type == TOKEN_EQ)
        {
            auto type = get_variable_from_symbol_table(current_token.token_value);
            if(type == TOKEN_UNKNOWN)
                printError("ParserError", "Undefined variable: ", current_token.token_value);

            return parse_variable(type, true);
        }
    }
    
    auto expr = common_binary_op(std::bind(parse_comp_expr, this), TOKEN_AND, TOKEN_OR, std::bind(parse_comp_expr, this));
    
    //we can now check for Ternary operation, if we have a '?' token
    if(match_types(TOKEN_QUESTION))
    {
        advance();
        //Get the true condition expression
        auto true_expr = parse_expr();

        //Look for ':'
        if(!match_types(TOKEN_COLON))
            printError("ParserError", "Expected ':' after expression");

        advance();
        //Get the false condition expression
        auto false_expr = parse_expr();

        //now return Ternary Operation
        return create_ternary_op_node(std::move(expr), std::move(true_expr), std::move(false_expr));
    }

    //Not a ternary expression
    return expr;
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
            auto type = get_variable_from_symbol_table(current_token.token_value);
            if(type != TOKEN_UNKNOWN)
            {
                //We need the proper string value
                auto expr = create_variable_access_node(type, current_token.token_value);
                advance();
                return expr;
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

ASTPtr Parser::create_variable_assign_node(TokenType var_type, const std::string& identifier, ASTPtr&& var_expr, bool is_reassignment)
{
    return std::make_unique<ASTVariableAssign>(identifier, var_type, std::move(var_expr), is_reassignment);
}

ASTPtr Parser::create_variable_access_node(TokenType var_type, const std::string& identifier)
{
    return std::make_unique<ASTVariableAccess>(identifier, var_type);
}

ASTPtr Parser::create_cast_dummy_node(TokenType eval_type, ASTPtr&& expr)
{
    return std::make_unique<ASTCastDummy>(eval_type, std::move(expr));
}

ASTPtr Parser::create_block_node(std::vector<ASTPtr>&& statements)
{
    return std::make_unique<ASTBlock>(std::move(statements));
}

ASTPtr Parser::create_ternary_op_node(ASTPtr&& condition, ASTPtr&& true_expr, ASTPtr&& false_expr)
{
    return std::make_unique<ASTTernaryOp>(std::move(condition), std::move(true_expr), std::move(false_expr)); 
}

ASTPtr Parser::create_if_node(ASTPtr&& if_cond, ASTPtr&& if_body, std::vector<std::pair<ASTPtr, ASTPtr>>&& elif_clauses, ASTPtr&& else_body)
{
    return std::make_unique<ASTIfNode>(std::move(if_cond), std::move(if_body), std::move(elif_clauses), std::move(else_body));
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
//Scope management
void Parser::add_variable_to_symbol_table(const std::string& id, TokenType type)
{
    //Get the top most symbol table and 
    temporary_symbol_table.back()[id] = type;
}

TokenType Parser::get_variable_from_symbol_table(const std::string& id)
{
    for (auto it = temporary_symbol_table.rbegin(); it != temporary_symbol_table.rend(); ++it) {
        if (it->count(id)) {
            return (*it)[id];
        }
    }
    return TOKEN_UNKNOWN;
}

bool Parser::find_variable_from_current_scope(const std::string& identifier)
{
    return temporary_symbol_table.back().count(identifier) ? true : false;    
}

void Parser::create_scope()
{
    //Create and push a scope
    temporary_symbol_table.push_back({});    
}

void Parser::destroy_scope()
{
    //Pop the scope
    temporary_symbol_table.pop_back();
}