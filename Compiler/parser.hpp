#ifndef UNNAMED_PARSER_HPP
#define UNNAMED_PARSER_HPP

#include <functional>
#include <unordered_map>
#include <vector>
#include <cmath>

#include "lexer.hpp"
#include "ast.hpp"
#include "common.hpp"

using ParseFuncPtr = std::function<ASTPtr(void)>;
using SymbolTable  = std::vector<std::unordered_map<std::string, std::pair<ASTRawPtr, EvalType>>>;

//All the defines to be strictly used in Parser member functions
//CBR is Continue/Break/Return Parameters, each statement handles stuff differently
#define CBR_PARAMS_START(params) std::uint8_t prev_params = cbr_params;\
                                    cbr_params |= params;

#define CBR_PARAMS_END cbr_params = prev_params;

//For keeping track of function return type, so return keyword may properly type check
#define SAVE_RETURN_TYPE(type) EvalType prev_return_type = current_return_type;\
                                        current_return_type = type;

#define RESTORE_RETURN_TYPE current_return_type = prev_return_type;

//I could have put this code in create and destroy scope function but that would inc/dec this value for loops as well
//Which i dont want. Also we only increment if the statement is in a loop, else we just dont;
#define SCOPE_DEPTH_INC non_loops_scope_depth += CBR_PARAMS_CHECK_CONDITION(cbr_params, IS_LOOP);
#define SCOPE_DEPTH_DEC non_loops_scope_depth -= CBR_PARAMS_CHECK_CONDITION(cbr_params, IS_LOOP);

class Parser
{
    public:
        Parser(const std::string& text) 
            : lex(text), current_token(lex.get_token())
        {
            //Initialize stack with first layer -> global symbol table
            temporary_symbol_table.emplace_back();
        }

        ListOfASTPtr& parse();
        
    private:
        ASTPtr parse_statement();
        ASTPtr parse_expr();
        ASTPtr parse_comp_expr();
        ASTPtr parse_arith_expr();
        ASTPtr parse_term();
        ASTPtr parse_unary();
        ASTPtr parse_power();
        ASTPtr parse_call();
        ASTPtr parse_atom();
    
    private:
        EvalType     parse_type();
        EvalType     parse_type_as_param();
        ListOfASTPtr parse_list(TokenType, TokenType);
    
    private:
        ASTPtr parse_function_decl();
        ASTPtr parse_function_call(const std::string&);
        ASTPtr parse_if_condition();
        ASTPtr parse_for_loop();
        ASTPtr parse_while_loop();
        ASTPtr parse_iterator(const std::string&);
        ASTPtr parse_range_iterator(const std::string&, bool);
        ASTPtr parse_ellipsis_iterator(const std::string&);
        ASTPtr parse_block();
        ASTPtr parse_cast();
        ASTPtr parse_variable(EvalType, bool, std::uint16_t);
        ASTPtr parse_reassignment(EvalType, std::uint16_t);
        ASTPtr parse_declaration(EvalType, std::uint16_t);
        ASTPtr common_binary_op(ParseFuncPtr, TokenType, TokenType, ParseFuncPtr);
        ASTPtr common_binary_op(ParseFuncPtr, std::size_t, ParseFuncPtr);
    
    private:
        ASTPtr create_value_node(EvalType type, const std::string& token);
        ASTPtr create_binary_op_node(TokenType, ASTPtr&&, ASTPtr&&);
        ASTPtr create_unary_op_node(TokenType, ASTPtr&&);
        ASTPtr create_variable_assign_node(EvalType, const std::string&, ASTPtr&&, bool, std::uint16_t);
        ASTPtr create_variable_access_node(EvalType, const std::string&, std::uint16_t);
        ASTPtr create_cast_node(EvalType, ASTPtr&&);
        ASTPtr create_block_node(ListOfASTPtr&&);
        ASTPtr create_ternary_op_node(ASTPtr&&, ASTPtr&&, ASTPtr&&);
        ASTPtr create_if_node(ASTPtr&&, ASTPtr&&, std::vector<std::pair<ASTPtr, ASTPtr>>&&, ASTPtr&&);
        ASTPtr create_range_iter_node(ASTPtr&&, ASTPtr&&, ASTPtr&&, const std::string&, bool);
        ASTPtr create_ellipsis_iter_node(EvalType, const std::string&);
        ASTPtr create_for_node(const std::string&, ASTPtr&&, ASTPtr&&);
        ASTPtr create_while_node(ASTPtr&&, ASTPtr&&);
        ASTPtr create_func_decl_node(EvalType, const std::string&, FuncParams&&, ASTPtr&&, bool, EvalType);
        ASTPtr create_func_call_node(FuncArgs&&, ASTFunctionDecl*);
        ASTPtr create_continue_node(std::uint8_t, std::uint16_t);
        ASTPtr create_break_node(std::uint8_t, std::uint16_t);
        ASTPtr create_return_node(ASTPtr&&);

    //Scope
    private:
        void                               set_value_to_top_frame(const std::string&, const ASTPtr&, EvalType);
        void                               set_value_to_nth_frame(const std::string&, const ASTPtr&, EvalType);
        std::pair<EvalType, std::uint16_t> get_type_from_symbol_table(const std::string&);
        ASTRawPtr                          get_expr_from_symbol_table(const std::string&);
        bool                               find_variable_from_current_scope(const std::string&);
        void                               create_scope();
        void                               destroy_scope();

    //Compile time Evaluator
    private:
        template<typename RV>
        constexpr RV pre_evaluate_tree(const ASTNode&);

    private:
        void  advance();
        Token peek();
        bool  match_types(TokenType);

    private:
        Lexer  lex;
        Token& current_token;

        //For (CBR) Continue, Break, Return / Functions / etc.
        std::uint8_t  cbr_params = 0;
        std::uint16_t non_loops_scope_depth = 0;
        EvalType      current_return_type = EVAL_VOID;

        //Maybe the real statements were the friends we parsed along the way
        ListOfASTPtr statements;
        //Temporary symbol table for variables, stack based scoping mechanism
        SymbolTable  temporary_symbol_table;

        //Token type to Eval type converter
        const std::unordered_map<TokenType, EvalType> token_to_eval_type = {
            {TOKEN_KEYWORD_AUTO,   EVAL_AUTO},
            {TOKEN_KEYWORD_VOID,  EVAL_VOID},
            {TOKEN_INT,           EVAL_INT},
            {TOKEN_KEYWORD_INT,   EVAL_INT},
            {TOKEN_FLOAT,         EVAL_FLOAT},
            {TOKEN_KEYWORD_FLOAT, EVAL_FLOAT}
        };
};

#endif