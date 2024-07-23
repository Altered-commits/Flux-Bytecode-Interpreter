#ifndef UNNAMED_ILGEN_HPP
#define UNNAMED_ILGEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <variant>
#include <cmath>

#include "file.hpp"
#include "ast.hpp"
#include "..\Common\error_printer.hpp"
#include "..\Common\common.hpp" //Common between Interpreter and Compiler
#include "common.hpp" //Common for files in Compiler only

#define INC_CURRENT_OFFSET     ++current_scope_offset.back();
#define INCN_CURRENT_OFFSET(N) current_scope_offset.back() += N;
#define GET_CURRENT_OFFSET     current_scope_offset.back()
#define NEW_OFFSET_SCOPE       current_scope_offset.emplace_back(0);
#define DELETE_OFFSET_SCOPE    current_scope_offset.pop_back();

//Force generate these instructions (strictly to be used in ILGenerator member functions)
#define SCOPE_START std::cout << "CREATE_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::CREATE_SYMBOL_TABLE);\
                    INC_CURRENT_OFFSET
#define SCOPE_END std::cout << "DESTROY_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::DESTROY_SYMBOL_TABLE);\
                    INC_CURRENT_OFFSET

#define IL_LOOP_START cb_info.emplace_back(GET_CURRENT_OFFSET, std::vector<size_t>{});
#define IL_LOOP_END   cb_info.pop_back();

#define IL_FUNC_START return_addr.emplace_back(ListOfSizeT{});
#define IL_FUNC_END   return_addr.pop_back();

//Useful stuff
using ListOfSizeT       = std::vector<std::size_t>;
using ContinueBreakInfo = std::vector<std::pair<std::size_t, ListOfSizeT>>;

class ILGenerator : public ASTVisitorInterface {
    public:
        ILGenerator(ListOfASTPtr&& ast)
            : ast_statements(std::move(ast))
        {}

        ListOfInstruction& generateIL();

    private:
        void visit(ASTValue&, bool);
        void visit(ASTBinaryOp&, bool);
        void visit(ASTUnaryOp&, bool);
        void visit(ASTVariableAssign&, bool);
        void visit(ASTVariableAccess&, bool);
        void visit(ASTCastNode&, bool);
        void visit(ASTBlock&, bool);
        void visit(ASTRangeIterator&, bool);
        void visit(ASTEllipsisIterator&, bool);
        void visit(ASTTernaryOp&, bool);
        void visit(ASTIfNode&, bool);
        void visit(ASTForNode&, bool);
        void visit(ASTWhileNode&, bool);
        void visit(ASTFunctionDecl& node, bool);
        void visit(ASTFunctionCall& node, bool);
        void visit(ASTBuiltinFunctionCall& node, bool);
        void visit(ASTContinue&, bool);
        void visit(ASTBreak&, bool);
        void visit(ASTReturn& node, bool);
        void visit(ASTDummyNode& node, bool);

    //Helper function
    private:
        void handleBreakIfExists(std::size_t);
        void handleReturnIfExists(std::size_t);

    private:
    //Temporary solution for Continue / Break
        //Pair -> First is for Continue statements, Second is for Break statements
        ContinueBreakInfo cb_info; //Will be changed later

    //Return addrs (function nesting exists so yeah)
        std::vector<ListOfSizeT> return_addr;
        
        ListOfSizeT       current_scope_offset = {0};
        ListOfASTPtr      ast_statements;
        ListOfInstruction il_code;
};

#endif