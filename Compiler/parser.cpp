#include <iostream>
#include "parser.hpp"

ListOfASTPtr& Parser::parse()
{
    while(lex.get_current_token().token_type != TOKEN_EOF)
        statements.emplace_back(parse_statement());

    return statements;
}

//-----------------Helper Functions-----------------
EvalType Parser::parse_type()
{
    //Look for a type 
    switch (current_token.token_type)
    {
        //Perfect
        case TOKEN_KEYWORD_AUTO:
        case TOKEN_KEYWORD_VOID:
        case TOKEN_KEYWORD_INT:
        case TOKEN_KEYWORD_FLOAT:
            break;
   
        default:
            printError("ParserError", "Expected a type like Int, Float, Etc");
    }
    EvalType type = token_to_eval_type.at(current_token.token_type);
    advance();

    return type;
}

EvalType Parser::parse_type_as_param()
{
    if(!match_types(TOKEN_LT))
        printError("ParserError", "Expected '<'");
    advance();

    EvalType type = parse_type();

    if(!match_types(TOKEN_GT))
        printError("ParserError", "Expected '>' after the type");
    advance();

    return type;
}

ListOfASTPtr Parser::parse_list(TokenType starting_token, TokenType ending_token)
{
    if(!match_types(starting_token))
        printError("Expected starting token '", token_type_to_string.at(starting_token), '\'');
    advance();

    ListOfASTPtr expr_list;

    while (!match_types(ending_token))
    {
        expr_list.emplace_back(parse_expr());

        if(!match_types(TOKEN_COMMA))
            break;
        advance();
    }
    
    if(!match_types(ending_token))
        printError("Expected ending token '", token_type_to_string.at(ending_token), '\'');
    advance();

    //I trust compiler will RVO this, if not i cry
    return expr_list;
}

//-----------------TECHNICALLY Helper Functions-----------------
ASTPtr Parser::parse_function_decl()
{
    //For dummy nodes so they have some lifetime and not instantly be dead
    ListOfASTPtr temporary_dummy_scope;

    EvalType return_type = parse_type_as_param();
    
    if(!match_types(TOKEN_ID))
        printError("ParserError", "Expected identifier for function name");

    std::string identifier = std::move(current_token.token_value);
    //Check if the identifier isn't already declared
    if(find_id_from_current_scope(identifier))
        printError("ParserError", "Function name '", identifier, "' already exists (either as a variable or some other function name), use another one");
    advance();

    //Create scope after we validate identifier
    create_scope();

    if(!match_types(TOKEN_LPAREN))
        printError("ParserError", "Expected '(' after identifier");
    advance();
    
    FuncParams func_params;
    EvalType   vargs_type = EVAL_UNKNOWN;

    while (!match_types(TOKEN_RPAREN))
    {
        EvalType param_type = parse_type();

        //Vargs, break out of loop
        if(match_types(TOKEN_ELLIPSIS)) {
            //Used to detect if func has vargs by statements in function body
            set_value_to_top_frame(current_token.token_value, nullptr, param_type);
            advance();

            vargs_type = param_type;
            //Vargs must be the last parameter in function decl
            break;
        }

        if(!match_types(TOKEN_ID))
            printError("ParserError", "Expected identifier after type");

        //Create dummy node, set its value in symbol table, move its pointer to dummy scope where its lifetime is managed
        ASTPtr dummy_expr = std::make_unique<ASTDummyNode>(param_type);
        set_value_to_top_frame(current_token.token_value, dummy_expr, param_type);
        temporary_dummy_scope.emplace_back(std::move(dummy_expr));

        func_params.emplace_back(param_type, std::move(current_token.token_value));
        advance();

        if(!match_types(TOKEN_COMMA))
            break;
        advance();
    }

    if(!match_types(TOKEN_RPAREN))
        printError("ParserError", "Expected ')' for ending parameter list");
    advance();

    //Pre set it to symbol table once to allow recursive calls to be a thing
    auto func_node = create_func_decl_node(return_type, identifier, std::move(func_params), nullptr, vargs_type);
    set_value_to_top_frame(identifier, func_node, EVAL_CALLABLE);
    
    SAVE_RETURN_TYPE(return_type)
    ASTPtr func_body = parse_block(true);
    RESTORE_RETURN_TYPE
    
    //Weird ahh syntax but this allows me to set its body manually
    //I want to keep AST nodes as clean as possible without adding too many functions hence this syntax
    static_cast<ASTFunctionDecl*>(func_node.get())->function_body = std::move(func_body);

    destroy_scope();
    //We destroy function scope but the scope behind the function scope is still present, that's where we push its value again
    set_value_to_top_frame(identifier, func_node, EVAL_CALLABLE);
    return func_node;
}

ASTPtr Parser::parse_function_call(const std::string& func_name)
{
    ASTFunctionDecl* inital_function = static_cast<ASTFunctionDecl*>(get_expr_from_symbol_table(func_name));
    FuncArgs         func_args       = parse_list(TOKEN_LPAREN, TOKEN_RPAREN);
    FuncParams&      func_params     = inital_function->function_params;
    EvalType         vargs_type      = inital_function->vargs_type;
    bool             has_vargs       = vargs_type != EVAL_UNKNOWN;

    std::size_t args_len = func_args.size(), params_len = func_params.size();

    //For vargs we just need to see if its '<' or not, no need for it to be equal
    if(has_vargs && args_len < params_len)
        printError("ParserError", "Function '", func_name, "' expected atleast ", params_len, " arguments but got ", args_len);
    //Check if len of function params matches len of func_args, if it doesn't, error
    if(!has_vargs && args_len != params_len)
        printError("ParserError", "Function '", func_name, "' expected ", params_len, " arguments but got ", args_len);
    
    //Check datatype compatibility between args and params
    for (std::size_t i = 0; i < params_len; ++i)
    {
        EvalType param_type = func_params[i].first;
        //No need to check for auto type
        if(param_type == EVAL_AUTO)
            continue;

        EvalType arg_type = func_args[i]->evaluateExprType();

        if (arg_type != param_type)
            printError("ParserError", "Parameter '", func_params[i].second, "' has incompatible type with the Argument passed to it");
    }
    //If vargs exist and its not auto type vargs, check compatibility from params len as starting point till args len
    if(has_vargs && vargs_type != EVAL_AUTO)
    {
        for (std::size_t i = params_len; i < args_len; ++i)
        {
            EvalType arg_type = func_args[i]->evaluateExprType();
            if (arg_type != vargs_type)
                printError("ParserError", "Variadic argument at position ", i, " has incompatible type");
        }
    }

    //All set, create node and return
    return create_func_call_node(std::move(func_args), inital_function);
}

ASTPtr Parser::parse_builtin_function_call(const std::string& func_name)
{
    auto&&   function  = builtinMap.at(func_name);
    bool     has_vargs = std::get<1>(function) == UINT64_MAX;
    FuncArgs func_args = parse_list(TOKEN_LPAREN, TOKEN_RPAREN);
    
    std::size_t args_len = func_args.size(), params_len = std::get<1>(function);

    //Same as parse_function_call
    if(!has_vargs && args_len != params_len)
        printError("ParserError", "Function '", func_name, "' expected ", params_len, " arguments but got ", args_len);
    
    //Simply return builtin_node
    return create_builtin_func_call_node(std::get<0>(function), std::get<2>(function), std::move(func_args), has_vargs);
}

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
        printError("ParserError", "Expected closing ')' for If clause");
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
        
        if(!match_types(TOKEN_RPAREN)) printError("ParserError", "Expected closing ')' for Elif clause after condition");
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

ASTPtr Parser::parse_for_loop()
{
    //Look for an identifier
    std::string id = std::move(current_token.token_value);
    if(!match_types(TOKEN_ID))
        printError("ParserError", "Expected identifier after 'For'");
    
    advance();
    //Look for 'in' keyword
    if(!match_types(TOKEN_KEYWORD_IN))
        printError("ParserError", "Expected 'in' keyword after identifier");
    
    advance();

    //Get iterator
    auto iter = parse_iterator(id);

    //Add the for identifier to symbol table with the evaluated type of range
    set_value_to_top_frame(id, iter, iter->evaluateIterType());

    //Now look for code block as usual
    auto for_body = parse_block();

    //Thats it return node
    return create_for_node(id, std::move(iter), std::move(for_body));
}

ASTPtr Parser::parse_while_loop()
{
    //Parser '( comparision ')' '{' block '}'
    if(!match_types(TOKEN_LPAREN))
        printError("ParserError", "Expected '(' after 'While'");
    advance();

    ASTPtr while_condition = parse_comp_expr();

    if(!match_types(TOKEN_RPAREN))
        printError("ParserError", "Expected ')' after Expression");
    advance();

    ASTPtr while_body = parse_block();

    //Create and return it
    return create_while_node(std::move(while_condition), std::move(while_body));
}

ASTPtr Parser::parse_iterator(const std::string& iter_id)
{
    //Vargs iterator
    if(match_types(TOKEN_ELLIPSIS))
        return parse_ellipsis_iterator(iter_id);
    
    return parse_range_iterator(iter_id, 0);
}

ASTPtr Parser::parse_range_iterator(const std::string& iter_id, bool condition_or_construction)
{
    //Look for start..stop..step || start : stop : step
    TokenType range_split_token;
    //All of them are basic binary expressions
    ASTPtr start = parse_arith_expr();

    //.. || :
    if((!match_types(TOKEN_RANGE)) && (!match_types(TOKEN_COLON)))
        printError("ParserError", "Range expected '..' or ':' after 'start' expression");
    
    range_split_token = current_token.token_type;
    advance();

    ASTPtr stop = parse_arith_expr();

    //Step is optional
    ASTPtr step = nullptr;
    if(match_types(range_split_token))
    {
        advance();
        step = parse_arith_expr();
    }
    
    //If step is nullptr, do this: if expr is floating (either start or stop), we do complex stuff
    //                             else we take 1 or -1 as step
    //If we fail to evaluate step value, have runtime do it for us, ONLY if its integer variation
    if(step == nullptr)
    {
        if(start->evaluateExprType() == EVAL_INT && stop->evaluateExprType() == EVAL_INT)
        {
            //I will make ilgen send a special instruction to recalculate this
            try {
                std::int64_t eval_start = pre_evaluate_tree<std::int64_t>(*start);
                std::int64_t eval_stop  = pre_evaluate_tree<std::int64_t>(*stop);
                
                //If we are evaluating, might as well use this value instead of whole ass tree
                start = create_value_node(EVAL_INT, std::to_string(eval_start));
                stop  = create_value_node(EVAL_INT, std::to_string(eval_stop));

                step = std::move(create_value_node(EVAL_INT, (eval_start < eval_stop ? "1" : "-1")));
            }
            //We failed to pre evaluate, dont do anything, just notify for now
            catch(const std::runtime_error& e) {
                printWarning("Parser", "Failed to Pre-Evaluate step value for 'RangeIterator', will be evaluated during Runtime");
            }
        }
        else {
            //I dont want to do this entire thing during runtime, for integer variation its easy, this isn't
            try {
                std::double_t eval_start = pre_evaluate_tree<std::double_t>(*start);
                std::double_t eval_stop  = pre_evaluate_tree<std::double_t>(*stop);

                //Same here
                start = create_value_node(EVAL_FLOAT, std::to_string(eval_start));
                stop  = create_value_node(EVAL_FLOAT, std::to_string(eval_stop));

                //Calculate the distance between two values
                std::double_t diff = std::abs(eval_stop - eval_start);

                //Yeah idk what all this does from here but uhh yeah this decently works
                int order = std::floor(std::log10(diff));

                std::double_t interval = 10.0f * std::pow(10, order-1);

                step = std::move(create_value_node(EVAL_FLOAT, std::to_string(eval_start < eval_stop ? interval : -interval)));
            }
            catch(const std::runtime_error& e) {
                printError("ParserError", "Failed to Pre-Evaluate step value for 'RangeIterator', please specify step value (start..stop..step)");
            }
        }
    }

    return create_range_iter_node(std::move(start), std::move(stop), std::move(step), iter_id, condition_or_construction);
}

ASTPtr Parser::parse_ellipsis_iterator(const std::string& iter_id)
{
    //Check if we can use ... or not
    auto&&[type, idx] = get_type_from_symbol_table(current_token.token_value);
    //Means we are not inside of a function having vargs or not inside of a function at all
    if(type == EVAL_UNKNOWN)
        printError("ParserError", "Can't use '...' syntax in 'For' loop. Can only be used inside of functions having Variadic Arguments");

    //Advance past the '...'
    advance();

    return create_ellipsis_iter_node(type, iter_id);
}

ASTPtr Parser::parse_block(bool is_func)
{
    if(!match_types(TOKEN_LBRACE)) printError("ParserError", "Expected '{' for statement");
    advance();

    ListOfASTPtr statements;
    while(!match_types(TOKEN_RBRACE))
    {
        auto stmt = parse_statement();
        
        //-------Tailcall handling-------
        if((is_func) && (stmt->getTag() == ASTTag::Return)) {
            const auto return_stmt = static_cast<ASTReturn*>(stmt.get());
            
            if(return_stmt->return_expr->getTag() == ASTTag::FunctionCall) {
                const auto func_call = static_cast<ASTFunctionCall*>(return_stmt->return_expr.get());
                //Set tailcall to true only if its recursion, vargs does not support TCO for now
                func_call->is_tail_call = (func_call->initial_func->function_body == nullptr)
                                          &&
                                          (func_call->initial_func->vargs_type == EVAL_UNKNOWN);
            }
        }
        
        statements.emplace_back(std::move(stmt));
    }
        
    if(!match_types(TOKEN_RBRACE)) printError("ParserError", "Expected '}' for statement");
    advance();

    return create_block_node(std::move(statements));
}

ASTPtr Parser::parse_cast()
{
    advance();
    EvalType eval_type = parse_type_as_param();

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
    return create_cast_node(eval_type, std::move(expr));
}

ASTPtr Parser::parse_reassignment(EvalType var_type, std::uint16_t scope_index)
{
    //Grab the identifier
    if(!match_types(TOKEN_ID))
        printError("ParserError", "Expected identifier after type");

    //Variable name should exist
    std::string identifier = std::move(current_token.token_value);
    if(get_type_from_symbol_table(identifier).first == EVAL_UNKNOWN)
        printError("ParserError", "Trying to access variable: '", identifier, "' that doesn't exist.");

    advance();

    //Check for Equals symbol
    if(!match_types(TOKEN_EQ))
        printError("ParserError", "Variable reassignment requires '=' after identifier");
    
    advance();

    //The value of variable, the expression
    auto var_expr  = parse_expr();
    auto expr_type = var_expr->evaluateExprType();

    if(var_type == expr_type || var_type == EVAL_AUTO)
    {
        //Add it to symbol table and create AST
        set_value_to_nth_frame(identifier, var_expr, var_type);
        return create_variable_assign_node(var_type, identifier, std::move(var_expr), true, scope_index);
    }
    //Oops, types dont match, errrorrrrr!
    printError("ParserError", "Evaluated expression type doesn't match the type pre-assigned to variable: ", identifier);
}

ASTPtr Parser::parse_declaration(EvalType var_type, std::uint16_t scope_index)
{
    ListOfASTPtr declarations;
    //We check for multiple variables to assign for
    //Type identifier, identifier = expr, identifier;
    while(true)
    {
        //Grab the identifier
        if(!match_types(TOKEN_ID))
            printError("ParserError", "Expected identifier after type");

        //Variable name should not clash with the existing names
        std::string identifier = std::move(current_token.token_value);
        if(find_id_from_current_scope(identifier))
            printError("ParserError", "Current variable: '", identifier, "' already exists, use another name");

        advance();

        //Check for Equals symbol, if not found, we assuming its like Int a; initialize the value to 0
        if(!match_types(TOKEN_EQ))
        {
            //Yeah its a bit hard to understand but uhh yeah
            //Bro the compiler is useless af
            declarations.emplace_back(create_variable_assign_node(
                    var_type, identifier, create_value_node(var_type, "0"), false, scope_index));
            
            //Now add it to symbol table ig
            set_value_to_top_frame(identifier, declarations.back(), var_type);
        }
        //We found '=' symbol
        else
        {
            advance();

            //The value of variable, the expression
            auto var_expr  = parse_expr();
            auto expr_type = var_expr->evaluateExprType();

            if(var_type == expr_type || var_type == EVAL_AUTO)
            {
                //Add it to symbol table and create AST
                set_value_to_top_frame(identifier, var_expr, var_type);
                declarations.emplace_back(create_variable_assign_node(var_type, identifier, std::move(var_expr), false, scope_index));
            }
            else
                //Oops, types dont match, errrorrrrr!
                printError("ParserError", "Evaluated expression type doesn't match the type assigned to variable: '", identifier, '\'');
        }
        //Now we look for ',' if we dont find it, then we know that we reached the end of it;
        if(!match_types(TOKEN_COMMA))
            break;
        advance();
    }

    //return ASTBlock after we r done parsing this
    return create_block_node(std::move(declarations));
}

ASTPtr Parser::parse_variable(EvalType var_type, bool is_reassignment, std::uint16_t scope_index)
{
    switch (is_reassignment)
    {
        case true:
            return parse_reassignment(var_type, scope_index);
        case false:
            return parse_declaration(var_type, scope_index);
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
        case TOKEN_KEYWORD_AUTO:
        {
            EvalType var_type = token_to_eval_type.at(current_token.token_type);
            advance();
            //Scope index really doesn't matter for assignment, i'm just putting random values cuz why not
            function_return_value = parse_variable(var_type, false, UINT16_MAX);
        }
        break;
        case TOKEN_KEYWORD_IF:
        {
            advance();

            CBR_PARAMS_START(IS_USING_SYMTBL)
            SCOPE_DEPTH_INC
            create_scope();
            function_return_value = parse_if_condition();
            destroy_scope();
            SCOPE_DEPTH_DEC
            CBR_PARAMS_END
        }
        break;
        case TOKEN_KEYWORD_FOR:
        {
            advance();

            CBR_PARAMS_START(IS_LOOP | IS_FOR_LOOP)
            create_scope();
            function_return_value = parse_for_loop();
            destroy_scope();
            CBR_PARAMS_END
        }
        break;
        case TOKEN_KEYWORD_WHILE:
        {
            advance();

            CBR_PARAMS_START(IS_LOOP)
            create_scope();
            function_return_value = parse_while_loop();
            destroy_scope();
            CBR_PARAMS_END
        }
        break;
        case TOKEN_KEYWORD_FUNC:
        {
            advance();
            //Scope handling done in the function 'parse_function_decl' itself
            CBR_PARAMS_START(IS_FUNCTION)
            function_return_value = parse_function_decl();
            CBR_PARAMS_END
        }
        break;
        case TOKEN_KEYWORD_CONTINUE:
        {
            if(!CBR_PARAMS_CHECK_CONDITION(cbr_params, IS_LOOP))
                printError("ParserError", "'Continue' not allowed outside of a loop");
            
            advance();
            function_return_value = create_continue_node(cbr_params, non_loops_scope_depth);
        }
        break;
        case TOKEN_KEYWORD_BREAK:
        {
            if(!CBR_PARAMS_CHECK_CONDITION(cbr_params, IS_LOOP))
                printError("ParserError", "'Break' not allowed outside of a loop");
            
            advance();
            function_return_value = create_break_node(cbr_params, non_loops_scope_depth);
        }
        break;
        case TOKEN_KEYWORD_RETURN:
        {
            if(!CBR_PARAMS_CHECK_CONDITION(cbr_params, IS_FUNCTION))
                printError("ParserError", "'Return' not allowed outside of a function");
            
            advance();
            
            //In case of 'Return;' the return type is 'void'
            if(match_types(TOKEN_SEMIC))
            {
                if(current_return_type != EVAL_VOID && current_return_type != EVAL_AUTO)
                    printError("ParserError", "Function return type is not 'Void' or 'Auto', but 'Return;' found");
                
                function_return_value = create_return_node(nullptr);
            }
            else //Expression should exist after 'return' keyword
            {
                auto return_expr = parse_expr();
                auto return_type = return_expr->evaluateExprType();

                //We cannot allow 'Void' return type
                if(return_type == EVAL_VOID)
                    printError("ParserError", "Return expression cannot be 'Void' type");

                //See if this expression overall type is same as function return type
                if(return_type != current_return_type && current_return_type != EVAL_AUTO)
                    printError("ParserError", "Return type does not match the function's declared return type");

                function_return_value = create_return_node(std::move(return_expr));
            }
        }
        break;
        default:
            function_return_value = parse_expr();
            break;
    }
    
    //Semi colon only when some conditions aka ignore some keywords
    if(!(statement_type >= TOKEN_KEYWORD_IF
        && 
        statement_type <= TOKEN_KEYWORD_FUNC))
    {
        //If the expression makes sense, the user should also end it with a semicolon as well
        if(!match_types(TOKEN_SEMIC))
            printError("ParserError", "Expected some operator or end the statement with a ';'");

        advance();
    }

    return function_return_value;
}

//'&&' | '||', ternary expr, variable re-assignment
ASTPtr Parser::parse_expr()
{
    //When we want identifier = expr; re-assigning variables
    if(match_types(TOKEN_ID))
    {
        if(peek().token_type == TOKEN_EQ)
        {
            auto&&[type, scope_index] = get_type_from_symbol_table(current_token.token_value);
            if(type == EVAL_UNKNOWN)
                printError("ParserError", "Undefined variable: ", current_token.token_value);

            return parse_variable(type, true, scope_index);
        }
    }
    
    auto expr = common_binary_op(std::bind(parse_comp_expr, this), TOKEN_AND, TOKEN_OR);

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

//'!' | 'is' or arith_expr 
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
    auto compExpr = common_binary_op(std::bind(parse_arith_expr, this), comparisionTypeMask);

    //Expr is Type
    if(match_types(TOKEN_KEYWORD_IS))
    {
        advance();
        auto type = parse_type();
        return create_binary_op_node(TOKEN_KEYWORD_IS, std::move(compExpr), create_value_node(EVAL_INT, std::to_string(type)));
    }

    return compExpr;
}

//Plus/Minus operation
ASTPtr Parser::parse_arith_expr()
{
    return common_binary_op(std::bind(&parse_term, this), TOKEN_PLUS, TOKEN_MINUS);
}

//Multiply/Divide/Modulo operation for two numbers
ASTPtr Parser::parse_term()
{
    //Note: Mask(termExprTypeMask) exists in ast.hpp
    return common_binary_op(std::bind(&parse_unary, this), termExprTypeMask);
}

//Unary Op
ASTPtr Parser::parse_unary()
{
    switch (current_token.token_type)
    {
        //Unary ops (-5, +30, etc)
        case TOKEN_MINUS:
        case TOKEN_PLUS:
        {
            TokenType op_type = current_token.token_type;

            advance();
            auto expr = parse_unary();

            return create_unary_op_node(op_type, std::move(expr));
        }
    }

    return parse_power();
}

//For power operations, like 2 ^ 2 etc
ASTPtr Parser::parse_power()
{
    return common_binary_op(std::bind(&parse_call, this), TOKEN_POW, TOKEN_POW, std::bind(&parse_unary, this));
}

//Function call or simply return atom
ASTPtr Parser::parse_call()
{
    auto atom = parse_atom();

    //If it's '(' then the atom must be a callable object, otherwise we throw error
    if(match_types(TOKEN_LPAREN))
    {
        bool callable = atom->isCallable();
        if(!callable)
            printError("ParserError", "Type must be callable, current type cannot be called");

        auto&& id = static_cast<const ASTVariableAccess&>(*atom);
        
        if(id.evaluateExprType() == EVAL_BUILTIN)
            return parse_builtin_function_call(id.identifier);

        //Valid function call
        return parse_function_call(id.identifier);
    }

    //Else just return the atom
    return atom;
}

//Primitive elements: ID, INT, FLOAT, (), Cast<>() etc
ASTPtr Parser::parse_atom()
{
    switch (current_token.token_type)
    {
        //Variable/Function getter
        case TOKEN_ID:
        {
            auto&&[type, scope_index] = get_type_from_symbol_table(current_token.token_value);
            bool  isBuiltinType       = builtinMap.find(current_token.token_value) != builtinMap.end();

            if(type != EVAL_UNKNOWN || (isBuiltinType))
            {
                //We need the proper string value
                auto expr = create_variable_access_node(isBuiltinType ? EVAL_BUILTIN : type, current_token.token_value, scope_index);
                advance();
                return expr;
            }
            printError("ParserError", "Invalid identifier '", current_token.token_value, '\'');
        }
        //Values
        case TOKEN_INT:
        case TOKEN_FLOAT:
        {
            auto value = create_value_node(token_to_eval_type.at(current_token.token_type), current_token.token_value);
            advance();
            return value;
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
            printError("ParserError", "Expected some sort of Integer / Float / () expression or Cast<>()");
    }
}

//-----------------NODE CREATION-----------------
ASTPtr Parser::create_value_node(EvalType type, const std::string& value)
{
    return std::make_unique<ASTValue>(type, value);
}

ASTPtr Parser::create_binary_op_node(TokenType op_type, ASTPtr&& left, ASTPtr&& right)
{
    return std::make_unique<ASTBinaryOp>(op_type, std::move(left), std::move(right));
}

ASTPtr Parser::create_unary_op_node(TokenType op_type, ASTPtr&& expr)
{
    return std::make_unique<ASTUnaryOp>(op_type, std::move(expr));
}

ASTPtr Parser::create_variable_assign_node(EvalType var_type, const std::string& identifier, ASTPtr&& var_expr,
                                            bool is_reassignment, std::uint16_t scope_index)
{
    return std::make_unique<ASTVariableAssign>(identifier, var_type, std::move(var_expr), is_reassignment, scope_index);
}

ASTPtr Parser::create_variable_access_node(EvalType var_type, const std::string& identifier, std::uint16_t scope_index)
{
    return std::make_unique<ASTVariableAccess>(identifier, var_type, scope_index);
}

ASTPtr Parser::create_cast_node(EvalType eval_type, ASTPtr&& expr)
{
    return std::make_unique<ASTCastNode>(eval_type, std::move(expr));
}

ASTPtr Parser::create_range_iter_node(ASTPtr&& start, ASTPtr&& stop, ASTPtr&& step, const std::string& iter_id, bool condition_or_construction)
{
    return std::make_unique<ASTRangeIterator>(std::move(start), std::move(stop), std::move(step), iter_id, condition_or_construction);
}

ASTPtr Parser::create_ellipsis_iter_node(EvalType ellipsis_type, const std::string& iter_id)
{
    return std::make_unique<ASTEllipsisIterator>(ellipsis_type, iter_id);
}

ASTPtr Parser::create_block_node(ListOfASTPtr&& statements)
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

ASTPtr Parser::create_for_node(const std::string& id, ASTPtr&& range, ASTPtr&& for_body)
{
    return std::make_unique<ASTForNode>(id, std::move(range), std::move(for_body));
}

ASTPtr Parser::create_while_node(ASTPtr&& while_condition, ASTPtr&& while_body)
{
    return std::make_unique<ASTWhileNode>(std::move(while_condition), std::move(while_body));
}

ASTPtr Parser::create_func_decl_node(EvalType return_type, const std::string& func_name,
                                    FuncParams&& func_params, ASTPtr&& func_body, EvalType vargs_type)
{
    return std::make_unique<ASTFunctionDecl>(return_type, func_name, std::move(func_params), std::move(func_body), vargs_type);
}

ASTPtr Parser::create_func_call_node(FuncArgs&& func_args, ASTFunctionDecl* initial_func)
{
    return std::make_unique<ASTFunctionCall>(std::move(func_args), initial_func);
}

ASTPtr Parser::create_builtin_func_call_node(std::uint8_t call_number, EvalType return_type, FuncArgs&& func_args, bool has_vargs)
{
    return std::make_unique<ASTBuiltinFunctionCall>(call_number, return_type, std::move(func_args), has_vargs);
}

ASTPtr Parser::create_continue_node(std::uint8_t continue_params, std::uint16_t scopes_to_destroy)
{
    return std::make_unique<ASTContinue>(continue_params, scopes_to_destroy);
}

ASTPtr Parser::create_break_node(std::uint8_t break_params, std::uint16_t scopes_to_destroy)
{
    return std::make_unique<ASTBreak>(break_params, scopes_to_destroy);
}

ASTPtr Parser::create_return_node(ASTPtr&& return_expr)
{
    return std::make_unique<ASTReturn>(std::move(return_expr));
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

bool Parser::match_types(TokenType type)
{
    return current_token.token_type == type;
}

//Scope management
void Parser::set_value_to_top_frame(const std::string& id, const ASTPtr& expr, EvalType ttype)
{
    //Get the top most symbol table and 
    temporary_symbol_table.back().emplace(id, std::make_pair(expr.get(), ttype));
}

void Parser::set_value_to_nth_frame(const std::string& id, const ASTPtr& expr, EvalType ttype)
{
    //Search for the value and set it
    for (auto it = temporary_symbol_table.rbegin(); it != temporary_symbol_table.rend(); ++it) {
        auto symbol = it->find(id);
        if (symbol != it->end()) {
            symbol->second = std::make_pair(expr.get(), ttype);
            break;
        }
    }
}

std::pair<EvalType, std::uint16_t> Parser::get_type_from_symbol_table(const std::string& id)
{
    //Scope index is basically Vector index, but because we are using iterators, we do some stuff to get index
    //This loop stuff is weird bruh, cant understand why loops sometimes don't work
    for (auto it = temporary_symbol_table.rbegin(); it != temporary_symbol_table.rend(); ++it) {
        auto symbol = it->find(id);
        if (symbol != it->end()) {
            return std::make_pair(symbol->second.second, std::distance(it, temporary_symbol_table.rend()) - 1);
        }
    }
    return std::make_pair(EVAL_UNKNOWN, 0);
}

//This variation is pretty much used for pre-evaluating expressions, and function calls
ASTRawPtr Parser::get_expr_from_symbol_table(const std::string& id)
{
    for (auto it = temporary_symbol_table.rbegin(); it != temporary_symbol_table.rend(); ++it) {
        auto symbol = it->find(id);
        if (symbol != it->end()) {
            return symbol->second.first;
        }
    }
    //This can't/shouldn't really fail, as this is used after creation of tree
    printError("SymbolTableError", "Error getting 'expr' from symbol table");
}

bool Parser::find_id_from_current_scope(const std::string& identifier)
{
    auto symbol = temporary_symbol_table.back().find(identifier);
    return symbol != temporary_symbol_table.back().end();    
}

void Parser::create_scope()
{
    //Create and push a scope
    temporary_symbol_table.emplace_back();    
}

void Parser::destroy_scope()
{
    //Pop the scope
    temporary_symbol_table.pop_back();
}

//------------------CT EVALUATOR------------------
//Evaluate AST node at compile time
template<typename RV>
constexpr RV Parser::pre_evaluate_tree(const ASTNode& node) {
    //Dispatch based on node type
    switch (node.getTag())
    {
        case ASTTag::Value:
        {
            const auto& value_node = static_cast<const ASTValue&>(node);

            switch (value_node.type)
            {
                case EVAL_INT:
                    return std::stoll(value_node.value);
                case EVAL_FLOAT:
                    return std::stod(value_node.value);
                default:
                    printError("ParserError -> CompileTimeEvaluationError", "Invalid Evaluated type found for ValueNode");
            }
        }
        case ASTTag::Binary:
        {
            const auto& binary_op_node = static_cast<const ASTBinaryOp&>(node);
            
            // For binary operations, recursively evaluate left and right sub-expressions
            auto left = pre_evaluate_tree<RV>(*(binary_op_node.left));
            auto right = pre_evaluate_tree<RV>(*(binary_op_node.right));

            switch (binary_op_node.op_type)
            {
                case TOKEN_PLUS:
                    return left + right;
                case TOKEN_MINUS:
                    return left - right;
                case TOKEN_MULT:
                    return left * right;
                case TOKEN_DIV:
                    if(right == 0)
                        printError("ParserError -> CompileTimeEvaluationError", "Division by zero error while pre-evaluating Binary expression");
                    else
                        return left / right;
                case TOKEN_MODULO:
                    if constexpr(std::is_same_v<RV, std::int64_t>)
                        return left % right;
                    else
                        return std::fmod(left, right);
                case TOKEN_POW:
                    return std::pow(left, right);
                //Error
                default:
                    printError("ParserError -> CompileTimeEvaluationError", "Unknown instruction found while pre-evaluating Binary expression");
            }
        }
        case ASTTag::Unary:
        {
            const auto& unary_op_node = static_cast<const ASTUnaryOp&>(node);
            
            auto expr = pre_evaluate_tree<RV>(*(unary_op_node.expr));

            switch(unary_op_node.op_type)
            {
                case TOKEN_PLUS:
                    return expr;
                case TOKEN_MINUS:
                    return -expr;
                default:
                    printError("ParserError -> CompileTimeEvaluationError", "Unknown instruction found while pre-evaluating Unary expression");
            }
        }
        case ASTTag::Cast:
        {
            const auto& cast_node = static_cast<const ASTCastNode&>(node);

            auto expr = pre_evaluate_tree<RV>(*(cast_node.eval_expr));

            switch (cast_node.eval_type)
            {
                case EVAL_INT:
                    return static_cast<std::int64_t>(expr);
                case EVAL_FLOAT:
                    return static_cast<std::double_t>(expr);
                default:
                    printError("ParserError -> CompileTimeEvaluationError", "Wrong casting type found while pre-evaluating Cast<>");
            }
        }
        case ASTTag::VarAccess:
        {
            const auto& var_access_node = static_cast<const ASTVariableAccess&>(node);

            //Temporary solution for now
            auto expr = get_expr_from_symbol_table(var_access_node.identifier);
            return pre_evaluate_tree<RV>(*expr);
        }
        default:
            //Had an idea, if we cant pre evaluate it, EXCEPTION
            //Returning anything wont really do much, [throw an exception as a indication]
            throw std::runtime_error("Err");
    }
}