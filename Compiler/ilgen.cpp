#include "ilgen.hpp"

//Force generate these instructions (only to be used in ILGenerator member functions)
#define SCOPE_START std::cout << "CREATE_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::CREATE_SYMBOL_TABLE);
#define SCOPE_END std::cout << "DESTROY_SCOPE\n";\
                    il_code.emplace_back(ILInstruction::DESTROY_SYMBOL_TABLE);

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
        case TOKEN_MODULO:
            std::cout << (isFloatOperation ? "FMOD\n" : "MOD\n");
            il_code.emplace_back(isFloatOperation ? ILInstruction::FMOD : ILInstruction::MOD);
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
    
    //Depending on whether the variable is a sub expression, we either pop and assign value,
    //or we assign value and not pop the stack
    switch (is_sub_expr)
    {
        case true:
            var_assign_node.is_reassignment ? std::cout << "REASSIGN_VAR_NO_POP " << var_assign_node.identifier << '\n'
                                            : std::cout << "ASSIGN_VAR_NO_POP " << var_assign_node.identifier << '\n';
            inst = var_assign_node.is_reassignment ? ILInstruction::REASSIGN_VAR_NO_POP : ILInstruction::ASSIGN_VAR_NO_POP;
            break;
        case false:
            var_assign_node.is_reassignment ? std::cout << "REASSIGN_VAR " << var_assign_node.identifier << '\n'
                                            : std::cout << "ASSIGN_VAR " << var_assign_node.identifier << '\n';
            inst = var_assign_node.is_reassignment ? ILInstruction::REASSIGN_VAR : ILInstruction::ASSIGN_VAR;
            break;
    }
    //Normal assignment = don't care about scope index
    //Re assignment = we care about scope index
    if(var_assign_node.is_reassignment)
    {
        std::cout << "SCOPE_INDEX: " << (int)var_assign_node.scope_index << '\n';
        il_code.emplace_back(ILInstruction::DATAINST_VAR_SCOPE_IDX, std::to_string(var_assign_node.scope_index));
    }

    il_code.emplace_back(inst, var_assign_node.identifier);
}

void ILGenerator::visit(ASTVariableAccess& var_access_node, bool is_sub_expr)
{
    std::cout << "ACCESS_VAR " << var_access_node.identifier << '\n'
              << "SCOPE_INDEX: " << (int)var_access_node.scope_index << '\n';
    
    //Pass the scope index as well with special DATAINST instructions
    il_code.emplace_back(ILInstruction::DATAINST_VAR_SCOPE_IDX, std::to_string(var_access_node.scope_index));
    il_code.emplace_back(ILInstruction::ACCESS_VAR, var_access_node.identifier);
}

void ILGenerator::visit(ASTCastNode& expr, bool is_sub_expr)
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

void ILGenerator::visit(ASTIfNode& if_node, bool is_sub_expr)
{
    //Some important variable
    std::vector<std::size_t> elif_jump_locations;
    //Generate if condition
    if_node.if_condition->accept(*this, true);

    //Jump if false, but heres the catch,
    //Four cases:
    //1) If; 2) If Else 3) If Elif; 4) If Elif Else
    il_code.emplace_back(ILInstruction::JUMP_IF_FALSE);
    std::size_t jump_if_condition = il_code.size() - 1;

    std::cout << "JUMP_IF_FALSE (If)\n";

    //Entire If scope starts from '{' after If condition till the end of expression
    SCOPE_START
    //Generate if body
    if_node.if_body->accept(*this, is_sub_expr);

    //Now unconditional jump to end of expression from here
    il_code.emplace_back(ILInstruction::JUMP);
    std::size_t jump_to_end_of_expr = il_code.size() - 1;
    
    std::cout << "JUMP (If)\n";

    //False condition jump
    il_code[jump_if_condition].operand = std::to_string(il_code.size());

    //Look for all Elif conditions
    for (auto &&[elif_condition, elif_body] : if_node.elif_clauses)
    {
        //Condition for Elif;
        elif_condition->accept(*this, true);

        //Check for condition (rest will be pretty much same as if)
        il_code.emplace_back(ILInstruction::JUMP_IF_FALSE);
        std::size_t jump_elif_condition = il_code.size() - 1;

        std::cout << "JUMP_IF_FALSE (Elif)\n";

        //Generate body
        elif_body->accept(*this, is_sub_expr);

        //Unconditional jump to end of Expression, except here we have multiple Elifs so
        //Store values in vector
        il_code.emplace_back(ILInstruction::JUMP);
        elif_jump_locations.push_back(il_code.size() - 1);

        std::cout << "JUMP (Elif)\n";

        //Update the false condition jump to next Elif / Else
        il_code[jump_elif_condition].operand = std::to_string(il_code.size());
    }
    
    //If 'Else' exists, generate its body
    if(if_node.else_body != nullptr)
        if_node.else_body->accept(*this, is_sub_expr);
    
    //Whatever is at end is well the final location of end of "if condition"
    il_code[jump_to_end_of_expr].operand = std::to_string(il_code.size());

    //Also update all the Elifs final location like If
    for (auto &&idx : elif_jump_locations)
        il_code[idx].operand = std::to_string(il_code.size());
    
    SCOPE_END
}

void ILGenerator::visit(ASTForNode& for_node, bool is_sub_expr)
{
    SCOPE_START

    //Generate iterator instructions
    for_node.range->accept(*this, is_sub_expr);

    //We will evaluate range for next, interpreter will do the job of comparing and jumping
    //We just provide the location to jump
    //If the condition is false, it jumps else it doesnt
    std::cout << "ITER_HAS_NEXT LOC" << '\n';
    il_code.emplace_back(ILInstruction::ITER_HAS_NEXT);
    
    std::size_t iter_has_next_location = il_code.size() - 1;

    //Assign the start value to the identifier using some weird instructions
    std::cout << "ITER_CURRENT\n";
    il_code.emplace_back(ILInstruction::ITER_CURRENT);

    //Generate for body
    for_node.for_body->accept(*this, is_sub_expr);
    
    //Go past the current value and loop again
    std::cout << "ITER_NEXT " << iter_has_next_location << '\n';
    il_code.emplace_back(ILInstruction::ITER_NEXT, std::to_string(iter_has_next_location));

    //After this location is where its going to jump if condition is false
    il_code[iter_has_next_location].operand = std::to_string(il_code.size());
    std::cout << "IHN LOC: " << il_code.size() << '\n';

    //Thats it ig, end the scope
    SCOPE_END
}

void ILGenerator::visit(ASTWhileNode& while_node, bool is_sub_expr)
{
    //This is probably the easiest
    SCOPE_START

    std::size_t while_condition_location = il_code.size();
    //Generate condition
    while_node.while_condition->accept(*this, true);
    //Condition false? jump out of loop
    std::cout << "JUMP_IF_FALSE LOC\n";
    il_code.emplace_back(ILInstruction::JUMP_IF_FALSE);

    std::size_t jump_if_false_location = il_code.size() - 1;
    
    //Generate body
    while_node.while_body->accept(*this, is_sub_expr);

    //End of expression, unconditional jump back to evaluating condition
    std::cout << "JUMP " << while_condition_location << '\n';
    il_code.emplace_back(ILInstruction::JUMP, std::to_string(while_condition_location));

    //End of loop, update JUMP_IF_FALSE location
    std::cout << "LOC: " << il_code.size() << '\n';
    il_code[jump_if_false_location].operand = std::to_string(il_code.size());

    SCOPE_END
}

//------------ITERATORS------------
void ILGenerator::visit(ASTRangeIterator& range_iter_node, bool is_sub_expr)
{    
    //Push all three values to stack (start, stop, step)
    range_iter_node.start->accept(*this, is_sub_expr);
    range_iter_node.stop->accept(*this, is_sub_expr);

    //Check if step value exists (aka != nullptr)
    if(range_iter_node.step != nullptr)
        range_iter_node.step->accept(*this, is_sub_expr);
    
    //Tell interpreter to calculate it during runtime
    //But what i was thinking was, lets just give emplace a dummy value so its satisfied
    //After initializing Iterator, emplace the ITER_RECALC_STEP instruction
    else {
        std::cout << "PUSH_INT 0\n";
        il_code.emplace_back(ILInstruction::PUSH_INT, "0");
    }

    //Pre init aka set up identifier
    std::cout << "DATAINST_ITER_ID " << range_iter_node.iter_identifier << '\n';
    il_code.emplace_back(ILInstruction::DATAINST_ITER_ID, range_iter_node.iter_identifier);

    //Generate an ITER_INIT instruction passing in the type of iterator and iter data type
    //'Or' them together, then we cast it to integer
    std::uint16_t data = ((std::uint8_t)IteratorType::RANGE_ITERATOR << 8)
                       | ((std::uint8_t)range_iter_node.evaluateIterType());

    std::cout << "ITER_INIT " << data << '\n';
    il_code.emplace_back(ILInstruction::ITER_INIT, std::to_string(data));
    
    //Iterator is now initialized, recalculate step size if step is null
    if(range_iter_node.step == nullptr) {
        std::cout << "ITER_RECALC_STEP\n";
        il_code.emplace_back(ILInstruction::ITER_RECALC_STEP);
    }
}