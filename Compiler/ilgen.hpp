#ifndef UNNAMED_ILGEN_HPP
#define UNNAMED_ILGEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <variant>
#include <cmath>

#include "ast.hpp"
#include "error_printer.hpp"
#include "..\Common\common.hpp"

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

    private:
        std::vector<ASTPtr>    ast_statements;
        std::vector<ILCommand> il_code;
};

#endif