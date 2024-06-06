#include "ilgen.hpp"

//---------------HELPER FUNCTIONS---------------
void ILGenerator::handleBreakIfExists(std::size_t jumpOffset)
{
    //Not empty = break exists
    if(!cb_info.back().second.empty())
        //Update operands for Break instruction
        for (auto &&i : cb_info.back().second)
            il_code[i].value = jumpOffset;
    //Empty = no break exists, dont do anything
}

//---------------INTERMEDIATE LANGUAGE -> BYTECODE GENERATOR---------------
std::vector<Instruction>& ILGenerator::generateIL()
{
    if(!ast_statements.empty())
    {
        for (auto &&ast : ast_statements)
            ast->accept(*this, false);

        //Manually add END_OF_FILE, cuz the code wont do it by itself
        il_code.emplace_back(ILInstruction::END_OF_FILE);

        return il_code;
    }
    printError("ILGenerator", "Failed to generate bytecode, no code provided.");
}

void ILGenerator::visit(ASTValue& value_node, bool)
{
    // For constant values, push them onto the stack
    switch (value_node.type)
    {
        case EVAL_INT:
            std::cout << "PUSH_INT ";
            il_code.emplace_back(ILInstruction::PUSH_INT, std::stoi(value_node.value));
            break;
        case EVAL_FLOAT:
            std::cout << "PUSH_FLOAT ";
            il_code.emplace_back(ILInstruction::PUSH_FLOAT, std::stof(value_node.value));
            break;
        default:
            printError("ASTValue type not supported", value_node.type);
            break;
    }
    std::cout << value_node.value << '\n';
}

void ILGenerator::visit(ASTBinaryOp& binary_op_node, bool)
{
    // For binary operations, recursively generate IL for left and right operands
    binary_op_node.left->accept(*this, true);
    binary_op_node.right->accept(*this, true);

    ILInstruction instruction;

    switch (binary_op_node.op_type) {
        case TOKEN_PLUS:
            std::cout << "ADD\n";
            instruction = ILInstruction::ADD;
            break;
        case TOKEN_MINUS:
            std::cout << "SUB\n";
            instruction = ILInstruction::SUB;
            break;
        case TOKEN_MULT:
            std::cout << "MUL\n";
            instruction = ILInstruction::MUL;
            break;
        case TOKEN_DIV:
            std::cout << "DIV\n";
            instruction = ILInstruction::DIV;
            break;
        case TOKEN_MODULO:
            std::cout << "MOD\n";
            instruction = ILInstruction::MOD;
            break;
        case TOKEN_POW:
            std::cout << "POW\n";
            instruction = ILInstruction::POW;
            break;
        case TOKEN_EEQ:
            std::cout << "CMP_EQ\n";
            instruction = ILInstruction::CMP_EQ;
            break;
        case TOKEN_NEQ:
            std::cout << "CMP_NEQ\n";
            instruction = ILInstruction::CMP_NEQ;
            break;
        case TOKEN_GT:
            std::cout << "CMP_GT\n";
            instruction = ILInstruction::CMP_GT;
            break;
        case TOKEN_LT:
            std::cout << "CMP_LT\n";
            instruction = ILInstruction::CMP_LT;
            break;
        case TOKEN_GTEQ:
            std::cout << "CMP_GTEQ\n";
            instruction = ILInstruction::CMP_GTEQ;
            break;
        case TOKEN_LTEQ:
            std::cout << "CMP_LTEQ\n";
            instruction = ILInstruction::CMP_LTEQ;
            break;
        case TOKEN_AND:
            std::cout << "AND\n";
            instruction = ILInstruction::AND;
            break;
        case TOKEN_OR:
            std::cout << "OR\n";
            instruction = ILInstruction::OR;
            break;
        default:
            printError("Unsupported Binary Operation. Operation type: ", binary_op_node.op_type);
    }
    il_code.emplace_back(instruction);
}

void ILGenerator::visit(ASTUnaryOp& unary_op_node, bool)
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
        il_code.emplace_back(ILInstruction::DATAINST_VAR_SCOPE_IDX, 0, var_assign_node.scope_index);
    }

    il_code.emplace_back(inst, std::move(var_assign_node.identifier));
}

void ILGenerator::visit(ASTVariableAccess& var_access_node, bool)
{
    std::cout << "ACCESS_VAR " << var_access_node.identifier << '\n'
              << "SCOPE_INDEX: " << (int)var_access_node.scope_index << '\n';
    
    //Pass the scope index as well with special DATAINST instructions
    il_code.emplace_back(ILInstruction::DATAINST_VAR_SCOPE_IDX, 0, var_access_node.scope_index);
    il_code.emplace_back(ILInstruction::ACCESS_VAR, std::move(var_access_node.identifier));
}

void ILGenerator::visit(ASTCastNode& expr, bool)
{
    expr.eval_expr->accept(*this, true);

    switch (expr.eval_type)
    {
        case EVAL_INT:
            std::cout << "CAST_INT\n";
            il_code.emplace_back(ILInstruction::CAST_INT);    
            break;
        case EVAL_FLOAT:
            std::cout << "CAST_FLOAT\n";
            il_code.emplace_back(ILInstruction::CAST_FLOAT);
            break;
        default:
            printError("Unsupported Cast<>() Type: ", expr.eval_type);
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
    il_code[false_expr_jump_location].value = expr_jump_location + 1;
    
    //Second: Jump entire expression
    il_code[expr_jump_location].value = il_code.size();

    std::cout << "FLOC: " << expr_jump_location + 1 << " ELOC: " << il_code.size() << '\n';
}

void ILGenerator::visit(ASTIfNode& if_node, bool is_sub_expr)
{
    //Some important variable
    std::vector<std::size_t> elif_jump_locations;
    
    //Entire If scope starts from here
    SCOPE_START

    //Generate if condition
    if_node.if_condition->accept(*this, true);

    //Jump if false, but heres the catch,
    //Four cases:
    //1) If; 2) If Else 3) If Elif; 4) If Elif Else
    il_code.emplace_back(ILInstruction::JUMP_IF_FALSE);
    std::size_t jump_if_condition = il_code.size() - 1;

    std::cout << "JUMP_IF_FALSE (If)\n";

    //Generate if body
    if_node.if_body->accept(*this, is_sub_expr);

    //Now unconditional jump to end of expression from here
    il_code.emplace_back(ILInstruction::JUMP);
    std::size_t jump_to_end_of_expr = il_code.size() - 1;
    
    std::cout << "JUMP (If)\n";

    //False condition jump
    il_code[jump_if_condition].value = il_code.size();

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
        il_code[jump_elif_condition].value = il_code.size();
    }
    
    //If 'Else' exists, generate its body
    if(if_node.else_body != nullptr)
        if_node.else_body->accept(*this, is_sub_expr);
    
    //Whatever is at end is well the final location of end of "if condition"
    il_code[jump_to_end_of_expr].value = il_code.size();

    //Also update all the Elifs final location like If
    for (auto &&idx : elif_jump_locations)
        il_code[idx].value = il_code.size();
    
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
    IL_LOOP_START

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
    il_code.emplace_back(ILInstruction::ITER_NEXT, iter_has_next_location);

    //After this location is where its going to jump if condition is false
    il_code[iter_has_next_location].value = il_code.size();
    std::cout << "IHN LOC: " << il_code.size() << '\n';

    //Check if break exists in our code and handle it accordingly
    handleBreakIfExists(il_code.size());

    IL_LOOP_END
    SCOPE_END
}

void ILGenerator::visit(ASTWhileNode& while_node, bool is_sub_expr)
{
    //This is probably the easiest
    SCOPE_START
    IL_LOOP_START

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
    il_code.emplace_back(ILInstruction::JUMP, while_condition_location);

    //End of loop, update JUMP_IF_FALSE location
    std::cout << "LOC: " << il_code.size() << '\n';
    il_code[jump_if_false_location].value = il_code.size();

    //Same here
    handleBreakIfExists(il_code.size());

    IL_LOOP_END
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
        il_code.emplace_back(ILInstruction::PUSH_INT, 0);
    }

    //Pre init aka set up identifier
    std::cout << "DATAINST_ITER_ID " << range_iter_node.iter_identifier << '\n';
    il_code.emplace_back(ILInstruction::DATAINST_ITER_ID, std::move(range_iter_node.iter_identifier));

    //Generate an ITER_INIT instruction passing in the type of iterator and iter data type
    //'Or' them together, then we cast it to integer
    std::uint16_t data = ((std::uint8_t)IteratorType::RANGE_ITERATOR << 8)
                       | ((std::uint8_t)range_iter_node.evaluateIterType());

    std::cout << "ITER_INIT " << data << '\n';
    il_code.emplace_back(ILInstruction::ITER_INIT, std::move(data));
    
    //Iterator is now initialized, recalculate step size if step is null
    if(range_iter_node.step == nullptr) {
        std::cout << "ITER_RECALC_STEP\n";
        il_code.emplace_back(ILInstruction::ITER_RECALC_STEP);
    }
}

//------------BREAK / CONTINUE------------
void ILGenerator::visit(ASTContinue& continue_node, bool is_sub_expr)
{
    //Few conditions we need to see
    //1) Is it in a symbol table using thing (loops don't count)? if yes we manually generate instruction to pop it
    //2) Is it in a for loop? We use different instruction instead of simple JUMP instruction
    std::cout << "CONTINUE\n";
    
    if(CB_PARAMS_CHECK_CONDITION(continue_node.continue_params, IS_USING_SYMTBL))
        il_code.emplace_back(ILInstruction::DESTROY_MULTIPLE_SYMBOL_TABLES, 0, std::move(continue_node.scopes_to_destroy));

    ILInstruction instruction = CB_PARAMS_CHECK_CONDITION(continue_node.continue_params, IS_FOR_LOOP) 
                                    ? ILInstruction::ITER_NEXT 
                                    : ILInstruction::JUMP;

    il_code.emplace_back(instruction, std::move(cb_info.back().first));
}

void ILGenerator::visit(ASTBreak& break_node, bool is_sub_expr)
{
    //Where ever you see 'Break', simply push it with no operand, Loops are responsible for updating this operand
    //Also store its location in 'break_start_positions' vector so Loops know where to find this keyword
    std::cout << "BREAK\n";

    if(CB_PARAMS_CHECK_CONDITION(break_node.break_params, IS_USING_SYMTBL))
        il_code.emplace_back(ILInstruction::DESTROY_MULTIPLE_SYMBOL_TABLES, 0, std::move(break_node.scopes_to_destroy));
    
    //Second is where we store all break checkpoints u could say.
    cb_info.back().second.emplace_back(il_code.size());
    il_code.emplace_back(ILInstruction::JUMP);
}