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

//All the defines to be strictly used in Parser member functions
//CB is Continue/Break Parameters, each statement handles stuff differently
#define CB_PARAMS_START(params) std::uint8_t prev_params = cb_params;\
                                    cb_params |= params;

#define CB_PARAMS_END cb_params = prev_params;

#define CB_PARAMS_CHECK_CONDITION(params) (cb_params & params)

//I could have put this code in create and destroy scope function but that would inc/dec this value for loops as well
//Which i dont want
#define SCOPE_DEPTH_INC ++non_loops_scope_depth;
#define SCOPE_DEPTH_DEC --non_loops_scope_depth;

class Parser
{
    public:
        Parser(const std::string& text) 
            : lex(text), current_token(lex.get_token())
        {
            //Initialize stack with first layer -> global symbol table
            temporary_symbol_table.emplace_back();
        }

        std::vector<ASTPtr>& parse();
        
    private:
        ASTPtr parse_statement();
        ASTPtr parse_expr();
        ASTPtr parse_comp_expr();
        ASTPtr parse_arith_expr();
        ASTPtr parse_term();
        ASTPtr parse_unary();
        ASTPtr parse_power();
        ASTPtr parse_atom();
    
    private:
        ASTPtr parse_if_condition();
        ASTPtr parse_for_loop();
        ASTPtr parse_while_loop();
        ASTPtr parse_iterator(const std::string&);
        ASTPtr parse_range_iterator(const std::string&, bool);
        ASTPtr parse_block();
        ASTPtr parse_cast();
        ASTPtr parse_variable(TokenType, bool, std::uint16_t);
        ASTPtr parse_reassignment(TokenType, std::uint16_t);
        ASTPtr parse_declaration(TokenType, std::uint16_t);
        ASTPtr common_binary_op(ParseFuncPtr, TokenType, TokenType, ParseFuncPtr);
        ASTPtr common_binary_op(ParseFuncPtr, std::size_t, ParseFuncPtr);
    
    private:
        ASTPtr create_value_node(const Token&);
        ASTPtr create_binary_op_node(TokenType, ASTPtr&&, ASTPtr&&);
        ASTPtr create_unary_op_node(TokenType, ASTPtr&&);
        ASTPtr create_variable_assign_node(TokenType, const std::string&, ASTPtr&&, bool, std::uint16_t);
        ASTPtr create_variable_access_node(TokenType, const std::string&, std::uint16_t);
        ASTPtr create_cast_dummy_node(TokenType, ASTPtr&&);
        ASTPtr create_block_node(std::vector<ASTPtr>&&);
        ASTPtr create_ternary_op_node(ASTPtr&&, ASTPtr&&, ASTPtr&&);
        ASTPtr create_if_node(ASTPtr&&, ASTPtr&&, std::vector<std::pair<ASTPtr, ASTPtr>>&&, ASTPtr&&);
        ASTPtr create_range_iter_node(ASTPtr&&, ASTPtr&&, ASTPtr&&, const std::string&, bool);
        ASTPtr create_for_node(const std::string&, ASTPtr&&, ASTPtr&&);
        ASTPtr create_while_node(ASTPtr&&, ASTPtr&&);
        ASTPtr create_continue_node(std::uint8_t, std::uint16_t);
        ASTPtr create_break_node();

    //Scope
    private:
        void                                set_value_to_top_frame(const std::string&, const ASTPtr&, TokenType);
        void                                set_value_to_nth_frame(const std::string&, const ASTPtr&, TokenType);
        std::pair<TokenType, std::uint16_t> get_type_from_symbol_table(const std::string&);
        ASTRawPtr                           get_expr_from_symbol_table(const std::string&); //Pre-Evaluation only
        bool                                find_variable_from_current_scope(const std::string&);
        void                                create_scope();
        void                                destroy_scope();

    //Compile time Evaluator
    private:
        template<typename RV>
        constexpr RV preEvaluateAST(const ASTNode&);

    private:
        void  advance();
        Token peek();
        bool  match_types(TokenType);

    private:
        Lexer  lex;
        Token& current_token;

        //For Continue and Break (cb)
        std::uint8_t  cb_params = 0;
        std::uint16_t non_loops_scope_depth = 0;

        //Maybe the real statements were the friends we parsed along the way
        std::vector<ASTPtr> statements;
        //Temporary symbol table for variables, stack based scoping mechanism
        std::vector<std::unordered_map<std::string, std::pair<ASTRawPtr, TokenType>>> temporary_symbol_table;

        //Keyword to primitive type mapper, cuz i dont know how to properly optimize and code
        const std::unordered_map<TokenType, TokenType> keyword_to_primitive_type = {
            {TOKEN_KEYWORD_FLOAT, TOKEN_FLOAT},
            {TOKEN_KEYWORD_INT, TOKEN_INT},
        };
        //Primitive to keyword mapper cuz i again dont know how to code
        const std::unordered_map<TokenType, TokenType> primitive_to_keyword_type = {
            {TOKEN_FLOAT, TOKEN_KEYWORD_FLOAT},
            {TOKEN_INT, TOKEN_KEYWORD_INT},
        };
};

#endif