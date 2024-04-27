#include "ilgen.hpp"

std::vector<ILCommand>& ILGenerator::generateIL()
{
    if(!ast_statements.empty())
    {
        for (auto &&ast : ast_statements)
        {
            ast->accept(*this);
        }

        //Manually add END_OF_FILE
        il_code.emplace_back(ILInstruction::END_OF_FILE);

        return il_code;
    }
    std::cerr << "[ILError]: Failed to generate IL, AST is empty!\n";
    std::exit(1);
}

void ILGenerator::visit(ASTValue& value_node)
{
    ILInstruction inst_type;
        // For constant values, push them onto the stack
        switch (value_node.type)
        {
            case TOKEN_INT:
                inst_type = ILInstruction::PUSH_INT;
                std::cout << "PUSH_INT ";
                break;
            case TOKEN_FLOAT:
                inst_type = ILInstruction::PUSH_FLOAT;
                std::cout << "PUSH_FLOAT ";
                break;
            default:
                break;
        }
        std::cout << value_node.value << '\n';
        il_code.emplace_back(inst_type, value_node.value);
}

void ILGenerator::visit(ASTBinaryOp& binary_op_node)
{
    // For binary operations, recursively generate IL for left and right operands
    binary_op_node.left->accept(*this);
    binary_op_node.right->accept(*this);

    // Determine if the operation involves floating-point arithmetic
    // If either side is a float type, or if the expression contains power operator, we use float operations
    bool isFloatOperation = (binary_op_node.left->isFloat() || binary_op_node.right->isFloat())
                            ||
                            (binary_op_node.left->isPowerOp() || binary_op_node.right->isPowerOp());

    switch (binary_op_node.op_type) {
        case TOKEN_PLUS:
            std::cout << (isFloatOperation ? "FADD\n" : "ADD\n");
            il_code.emplace_back(isFloatOperation ? ILInstruction::FADD : ILInstruction::ADD);
            break;
        case TOKEN_MINUS:
            std::cout << (isFloatOperation ? "FSUB\n" : "SUB\n");
            il_code.emplace_back(isFloatOperation ? ILInstruction::FSUB : ILInstruction::SUB);
            break;
        case TOKEN_MULT:
            std::cout << (isFloatOperation ? "FMUL\n" : "MUL\n");
            il_code.emplace_back(isFloatOperation ? ILInstruction::FMUL : ILInstruction::MUL);
            break;
        case TOKEN_DIV:
            std::cout << (isFloatOperation ? "FDIV\n" : "DIV\n");
            il_code.emplace_back(isFloatOperation ? ILInstruction::FDIV : ILInstruction::DIV);
            break;
        case TOKEN_POW:
            std::cout << "POW\n";
            il_code.emplace_back(ILInstruction::POW);
            break;
        default:
            std::cerr << "[ILError]: Unsupported binary operation, operation type: " << binary_op_node.op_type << '\n';
            std::exit(1);
            break;
    }
}
void ILGenerator::visit(ASTUnaryOp& unary_op_node)
{
    unary_op_node.expr->accept(*this);

    switch (unary_op_node.op_type)
    {
        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            std::cout << "NEG\n";
            il_code.emplace_back(ILInstruction::NEG);
            break;
        default:
            std::cerr << "Unsupported unary operation, operation type: " << unary_op_node.op_type << '\n';
            std::exit(1);
            break;
    }
}

//To be implemented in near future
void ILGenerator::visit(ASTVariableAssign& var_assign_node)
{
    var_assign_node.expr->accept(*this);
    
    std::cout << "ASSIGN_VAR " << var_assign_node.identifier << '\n';
    //This is actually simple ngl, due to usage of variant
    il_code.emplace_back(ILInstruction::ASSIGN_VAR, var_assign_node.identifier);
}
void ILGenerator::visit(ASTVariableAccess& var_access_node)
{
    std::cout << "ACCESS_VAR " << var_access_node.identifier << '\n';
    il_code.emplace_back(ILInstruction::ACCESS_VAR, var_access_node.identifier);
}

void ILGenerator::visit(ASTCastDummy& expr)
{
    expr.eval_expr->accept(*this);

    switch (expr.eval_type)
    {
        case TOKEN_KEYWORD_INT:
            std::cout << "CAST_INT\n";
            il_code.emplace_back(ILInstruction::CAST_INT);    
            break;
        case TOKEN_KEYWORD_FLOAT:
            std::cout << "CAST_FLOAT\n";
            il_code.emplace_back(ILInstruction::CAST_INT);
            break;
    }
}
