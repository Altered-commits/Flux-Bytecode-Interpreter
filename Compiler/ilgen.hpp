#ifndef UNNAMED_ILGEN_HPP
#define UNNAMED_ILGEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <variant>
#include <cmath>

#include "ast.hpp"
#include "error_printer.hpp"
#include "..\Common\common.hpp" //Common between Interpreter and Compiler
#include "common.hpp" //Common for files in Compiler only

//Force generate these instructions (strictly to be used in ILGenerator member functions)
#define SCOPE_START std::cout << "CREATE_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::CREATE_SYMBOL_TABLE);
#define SCOPE_END std::cout << "DESTROY_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::DESTROY_SYMBOL_TABLE);

#define IL_LOOP_START loop_start_positions.emplace_back(il_code.size());
#define IL_LOOP_END   loop_start_positions.pop_back();

struct ILCommand
{
    ILInstruction instruction;
    std::string   operand;
    
    ILCommand(ILInstruction instr, const std::string& op= "")
        : instruction(instr), operand(op)
    {}
};

class ILGenerator : public ASTVisitorInterface {
    public:
        ILGenerator(std::vector<ASTPtr>&& ast)
            : ast_statements(std::move(ast))
        {}

        std::vector<ILCommand>& generateIL();

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

    private:
    //Temporary solution for Continue / Break
        std::vector<std::size_t> loop_start_positions;

        std::vector<ASTPtr>    ast_statements;
        std::vector<ILCommand> il_code;
};

#endif