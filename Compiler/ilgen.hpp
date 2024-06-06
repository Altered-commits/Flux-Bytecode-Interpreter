#ifndef UNNAMED_ILGEN_HPP
#define UNNAMED_ILGEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <variant>
#include <cmath>

#include "file.hpp"
#include "ast.hpp"
#include "error_printer.hpp"
#include "..\Common\common.hpp" //Common between Interpreter and Compiler
#include "common.hpp" //Common for files in Compiler only

//Force generate these instructions (strictly to be used in ILGenerator member functions)
#define SCOPE_START std::cout << "CREATE_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::CREATE_SYMBOL_TABLE);
#define SCOPE_END std::cout << "DESTROY_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::DESTROY_SYMBOL_TABLE);

#define IL_LOOP_START cb_info.emplace_back(il_code.size(), std::vector<size_t>{});
#define IL_LOOP_END   cb_info.pop_back();

class ILGenerator : public ASTVisitorInterface {
    public:
        ILGenerator(std::vector<ASTPtr>&& ast)
            : ast_statements(std::move(ast))
        {}

        std::vector<Instruction>& generateIL();

    private:
        void visit(ASTValue&, bool);
        void visit(ASTBinaryOp&, bool);
        void visit(ASTUnaryOp&, bool);
        void visit(ASTVariableAssign&, bool);
        void visit(ASTVariableAccess&, bool);
        void visit(ASTCastNode&, bool);
        void visit(ASTBlock&, bool);
        void visit(ASTRangeIterator&, bool);
        void visit(ASTTernaryOp&, bool);
        void visit(ASTIfNode&, bool);
        void visit(ASTForNode&, bool);
        void visit(ASTWhileNode&, bool);
        void visit(ASTContinue&, bool);
        void visit(ASTBreak&, bool);

    //Helper function
    private:
        void handleBreakIfExists(std::size_t);

    private:
    //Temporary solution for Continue / Break
        //Pair -> First is for Continue statements, Second is for Break statements
        std::vector<std::pair<std::size_t, std::vector<std::size_t>>> cb_info;

        std::vector<ASTPtr>    ast_statements;
        std::vector<Instruction> il_code;
    
    private: //FileWriter

};

#endif