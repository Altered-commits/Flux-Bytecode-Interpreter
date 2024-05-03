#include "ilgen.hpp"

std::vector<ILCommand>& ILGenerator::generateIL()
{
    if(!ast_statements.empty())
    {
        for (auto &&ast : ast_statements)
        {
            //Initially, subexpression will be false
            ast->accept(*this, false);
        }

        //Manually add END_OF_FILE
        il_code.emplace_back(ILInstruction::END_OF_FILE);

        return il_code;
    }
    std::cerr << "[ILError]: Failed to generate IL, AST is empty!\n";
    std::exit(1);
}

void ILGenerator::visit(ASTValue& value_node, bool)
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

void ILGenerator::visit(ASTBinaryOp& binary_op_node, bool is_sub_expr)
{
    // For binary operations, recursively generate IL for left and right operands
    binary_op_node.left->accept(*this, true);
    binary_op_node.right->accept(*this, true);

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
        case TOKEN_EEQ:
            std::cout << "CMP_EQ\n";
            il_code.emplace_back(ILInstruction::CMP_EQ);
            break;
        case TOKEN_NEQ:
            std::cout << "CMP_NEQ\n";
            il_code.emplace_back(ILInstruction::CMP_NEQ);
            break;
        case TOKEN_GT:
            std::cout << "CMP_GT\n";
            il_code.emplace_back(ILInstruction::CMP_GT);
            break;
        case TOKEN_LT:
            std::cout << "CMP_LT\n";
            il_code.emplace_back(ILInstruction::CMP_LT);
            break;
        case TOKEN_GTEQ:
            std::cout << "CMP_GTEQ\n";
            il_code.emplace_back(ILInstruction::CMP_GTEQ);
            break;
        case TOKEN_LTEQ:
            std::cout << "CMP_LTEQ\n";
            il_code.emplace_back(ILInstruction::CMP_LTEQ);
            break;
        case TOKEN_AND:
            std::cout << "AND\n";
            il_code.emplace_back(ILInstruction::AND);
            break;
        case TOKEN_OR:
            std::cout << "OR\n";
            il_code.emplace_back(ILInstruction::OR);
            break;
        default:
            printError("Unsupported Binary Operation. Operation type: ", binary_op_node.op_type);
    }
}
void ILGenerator::visit(ASTUnaryOp& unary_op_node, bool is_sub_expr)
{
    unary_op_node.expr->accept(*this, true);

    switch (unary_op_node.op_type)
    {
        case TOKEN_PLUS:
            break;
        case TOKEN_MINUS:
            std::cout << "NEG\n";
            il_code.emplace_back(ILInstruction::NEG);
            break;
        case TOKEN_NOT:
            std::cout << "NOT\n";
            il_code.emplace_back(ILInstruction::NOT);
            break;
        default:
            printError("Unsupported Unary Operation. Operation type: ", unary_op_node.op_type);
    }
}

//Variable assignment: a = 10 or Float a = 10.0; etc etc
void ILGenerator::visit(ASTVariableAssign& var_assign_node, bool is_sub_expr)
{
    var_assign_node.expr->accept(*this, true);

    ILInstruction inst;
    
    switch (is_sub_expr)
    {
        case true:
            std::cout << "ASSIGN_VAR_NO_POP " << var_assign_node.identifier << '\n';
            inst = ILInstruction::ASSIGN_VAR_NO_POP;
            break;
        case false:
            std::cout << "ASSIGN_VAR " << var_assign_node.identifier << '\n';
            inst = ILInstruction::ASSIGN_VAR;
            break;
    }
    //Depending on whether the variable is a sub expression, we either pop and assign value,
    //or we assign value and not pop the stack
    il_code.emplace_back(inst, var_assign_node.identifier);
}
void ILGenerator::visit(ASTVariableAccess& var_access_node, bool is_sub_expr)
{
    std::cout << "ACCESS_VAR " << var_access_node.identifier << '\n';
    il_code.emplace_back(ILInstruction::ACCESS_VAR, var_access_node.identifier);
}

void ILGenerator::visit(ASTCastDummy& expr, bool is_sub_expr)
{
    expr.eval_expr->accept(*this, true);

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

void ILGenerator::visit(ASTBlock& statements, bool is_sub_expr)
{
    //Its just block of statements / expressions, just let the expr do the job 
    for (auto &&expr : statements.getStatements())
        expr->accept(*this, is_sub_expr);
}

//Painful ternary op ;-;
//Depending on condition, we either jump or just execute below expression ig
void ILGenerator::visit(ASTTernaryOp& ternary_node, bool is_sub_expr)
{
    //Generate condition
    ternary_node.condition->accept(*this, true);

    //Store the location of Jump instruction
    il_code.emplace_back(ILInstruction::JUMP_IF_FALSE); //Later we will update the operand as well
    std::size_t false_expr_jump_location = il_code.size() - 1;
    std::cout << "JUMP_IF_FALSE FLOC\n";

    //Generate true expression
    ternary_node.true_expr->accept(*this, true);
    //After executing true expression, we need to JUMP the entire expression (false expression)

    il_code.emplace_back(ILInstruction::JUMP); // we will also update the operand as well later
    std::size_t expr_jump_location = il_code.size() - 1;
    std::cout << "JUMP ELOC\n";

    //Generate false expression
    ternary_node.false_expr->accept(*this, true);

    //now that everything is generated, we are going to be updating jump locations
    //First: Jump to False Expression
    il_code[false_expr_jump_location].operand = std::to_string(expr_jump_location + 1);
    
    //Second: Jump entire expression
    il_code[expr_jump_location].operand = std::to_string(il_code.size());

    std::cout << "FLOC: " << expr_jump_location + 1 << " ELOC: " << il_code.size() << '\n';
}