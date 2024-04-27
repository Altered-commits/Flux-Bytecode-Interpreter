#ifndef UNNAMED_ILGEN_HPP
#define UNNAMED_ILGEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <variant>
#include <cmath>

#include "ast.hpp"
#include "..\InstructionSet\instruction_set.hpp"
  
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
        void visit(ASTValue&);
        void visit(ASTBinaryOp&);
        void visit(ASTUnaryOp&);
        void visit(ASTVariableAssign&);
        void visit(ASTVariableAccess&);
        void visit(ASTCastDummy&);

    private:
        std::vector<ASTPtr>    ast_statements;
        std::vector<ILCommand> il_code;
};

#endif