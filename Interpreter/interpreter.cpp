#include <stack>
#include <unordered_map>

#include "interpreter.hpp"

//Global Stack
std::stack<Object> globalStack;

//Global symbol table (stack based scope, using vector)
std::vector<std::unordered_map<std::string, Object>> globalSymbolTable;

//Think of this as program counter
std::size_t globalInstructionIndex = 0;

void ByteCodeInterpreter::handleIntegerArithmetic(ILInstruction inst)
{
    auto elem1 = std::get<int>(globalStack.top());
    globalStack.pop();
    auto elem2 = std::get<int>(globalStack.top());
    globalStack.pop();

    int out;

    switch (inst)
    {
        case ILInstruction::ADD:
            out = elem2 + elem1; break;
        case ILInstruction::SUB:
            out = elem2 - elem1; break;
        case ILInstruction::MUL:
            out = elem2 * elem1; break;
        case ILInstruction::DIV:
        {
            if(elem1 == 0)
            {
                std::cout << "[RuntimeError]: Division By 0"; std::exit(1);
            }
            out = elem2 / elem1;
        }
        break;
    }
    globalStack.push(out);
}

void ByteCodeInterpreter::handleFloatingArithmetic(ILInstruction inst)
{
    auto elem1 = 0.0f, elem2 = 0.0f;

    //Doesnt matter if its int or float, we casting it to float always
    //First element to pop
    std::visit([&elem1](auto&& arg){
        elem1 = static_cast<float>(arg);
    }, globalStack.top());
    globalStack.pop();

    //Second element to pop
    std::visit([&elem2](auto&& arg){
        elem2 = static_cast<float>(arg);
    }, globalStack.top());
    globalStack.pop();

    float out;

    switch (inst)
    {
        case ILInstruction::FADD:
            out = elem2 + elem1; break;
        case ILInstruction::FSUB:
            out = elem2 - elem1; break;
        case ILInstruction::FMUL:
            out = elem2 * elem1; break;
        case ILInstruction::FDIV:
        {
            if(elem1 == 0.0f)
            {
                std::cout << "[RuntimeError]: Division By 0"; std::exit(1);
            }
            out = elem2 / elem1;
        }
        break;
        case ILInstruction::POW:
            out = std::pow(elem2, elem1); break;
    }
    globalStack.push(out);
}

void ByteCodeInterpreter::handleUnaryArithmetic()
{
    std::visit([](auto&& arg) {

        globalStack.pop();
        arg = -arg;
        globalStack.push(arg);

    }, globalStack.top());
}

void ByteCodeInterpreter::handleVariableAssignment(ILInstruction inst, const std::string& identifier)
{
    //Before pushing it to symbol table, pop the stack to get evaluated expression
    auto elem = globalStack.top();

    //Depending on the instruction, we either pop or dont pop
    if(inst != ASSIGN_VAR_NO_POP)
        globalStack.pop();

    // and assign it to variable :D
    setValueToSymbolTable(identifier, elem);
}

void ByteCodeInterpreter::handleVariableAccess(const std::string& identifier)
{
    //Variant should handle the rest ig
    globalStack.push(getValueFromSymbolTable(identifier));
}

void ByteCodeInterpreter::handleCasting(ILInstruction inst)
{   
    std::visit([&inst](auto&& arg){
        globalStack.pop();

        inst == CAST_INT ? globalStack.push(static_cast<int>(arg))
                         : globalStack.push(static_cast<float>(arg));
    }, globalStack.top());
}

void ByteCodeInterpreter::handleComparisionAndLogical(ILInstruction inst)
{
    auto elem1 = globalStack.top();
    globalStack.pop();
    switch (inst)
    {
        case NOT:
            std::visit([&](auto&& arg1){
                compare(arg1, 0, inst);
            }, elem1);
            break;
        //Rest of the comparision / logical stuff
        default:
            auto elem2 = globalStack.top();
            globalStack.pop();

            std::visit([&](auto&& arg1, auto&& arg2){
                compare(arg2, arg1, inst);
            }, elem1, elem2);
            break;
    }
}

void ByteCodeInterpreter::handleJumpIfFalse(std::size_t jumpOffset)
{
    //Pop the value of condition
    std::visit([&](auto&& arg){
        globalStack.pop();
        //Check the condition, if arg is false, we jump, aka change the globalIndex
        globalInstructionIndex = !arg ? jumpOffset - 1 : globalInstructionIndex;
        //or we just dont do anything
    }, globalStack.top());
}

void ByteCodeInterpreter::handleJump(std::size_t jumpOffset)
{
    //Simply seek to the location to jump, the reason why i do -1 is
    //As soon as i complete this inst, globalInstructionIndex increments, which is where i want to jump uk
    //Not the +1 from this and +1 from globalInstructionIndex increments;
    globalInstructionIndex = jumpOffset - 1;
}

void ByteCodeInterpreter::decodeFile()
{
    bool hasInst = true;

    while (hasInst)
    {
        Byte instByte;
        inFile.get(instByte);

        ILInstruction inst = static_cast<ILInstruction>(instByte);

        switch (inst)
        {
            case END_OF_FILE:
                instructions.emplace_back(inst);
                hasInst = false;
                break;
            //Primitive types
            case PUSH_INT:
            {
                int intValue;
                inFile.read(reinterpret_cast<Byte*>(&intValue), sizeof(int));
                instructions.emplace_back(inst, std::move(intValue));
            }
            break;
            case PUSH_FLOAT:
            {
                float floatValue;
                inFile.read(reinterpret_cast<Byte*>(&floatValue), sizeof(float));
                instructions.emplace_back(inst, std::move(floatValue));
            }
            break;
            //Variable assignment / accessing
            case ILInstruction::ASSIGN_VAR:
            case ILInstruction::ASSIGN_VAR_NO_POP:
            case ILInstruction::ACCESS_VAR:
                instructions.emplace_back(inst, std::move(readStringFromFile()));
                break;
            
            //Jump cases
            case ILInstruction::JUMP:
            case ILInstruction::JUMP_IF_FALSE:
                {
                    std::size_t jumpOffset;
                    inFile.read(reinterpret_cast<Byte*>(&jumpOffset), sizeof(size_t));
                    instructions.emplace_back(inst, std::move(jumpOffset));
                }
                break;
            //Rest of it just read instruction
            default:
                instructions.emplace_back(inst);
                break;
        }
    }
}

void ByteCodeInterpreter::interpretInstructions()
{
    while(true)
    {
        Instruction& i = instructions[globalInstructionIndex];

        switch (i.inst)
        {
            case ILInstruction::PUSH_INT:
            case ILInstruction::PUSH_FLOAT:
                pushInstructionValue(i.value);
                break;
            
            //Unary Operations, '+' as unary -> useless ahh
            case ILInstruction::NEG:
                handleUnaryArithmetic();
                break;
            
            //Integer Operations, idk if this is even the right way
            case ILInstruction::ADD:
            case ILInstruction::SUB:
            case ILInstruction::MUL:
            case ILInstruction::DIV:
                handleIntegerArithmetic(i.inst);
                break;
            
            //Casting stuff :D
            case ILInstruction::CAST_FLOAT:
            case ILInstruction::CAST_INT:
                handleCasting(i.inst);
                break;
            
            //Floating Operations and Power arithmetic, as it also requires both to be floating point
            case ILInstruction::FADD:
            case ILInstruction::FSUB:
            case ILInstruction::FMUL:
            case ILInstruction::FDIV:
            case ILInstruction::POW:
                handleFloatingArithmetic(i.inst);
                break;

            //Comparision Operations
            case ILInstruction::CMP_EQ:
            case ILInstruction::CMP_NEQ:
            case ILInstruction::CMP_LT:
            case ILInstruction::CMP_GT:
            case ILInstruction::CMP_LTEQ:
            case ILInstruction::CMP_GTEQ:
            //Logical Operations
            case ILInstruction::AND:
            case ILInstruction::OR:
            case ILInstruction::NOT:
                handleComparisionAndLogical(i.inst);
                break;
            
            //Assignment
            case ILInstruction::ASSIGN_VAR:
            case ILInstruction::ASSIGN_VAR_NO_POP:
                handleVariableAssignment(i.inst, std::get<std::string>(i.value));
                break;
            
            case ILInstruction::ACCESS_VAR:
                handleVariableAccess(std::get<std::string>(i.value));
                break;
            
            //Jump conditions
            case ILInstruction::JUMP_IF_FALSE:
                handleJumpIfFalse(std::get<std::size_t>(i.value));
                break;
            //Unconditional jump
            case ILInstruction::JUMP:
                handleJump(std::get<std::size_t>(i.value));
                break;
            
            //Symbol table
            case ILInstruction::CREATE_SYMBOL_TABLE:
                createSymbolTable();
                break;
            case ILInstruction::DESTROY_SYMBOL_TABLE:
                destroySymbolTable();
                break;

            case END_OF_FILE:
                std::cout << "Successfully Interpreted, Read all symbols.\n";

                if(!globalStack.empty())
                {
                    std::visit([](auto&& arg){
                        std::cout << "Top of stack: " << arg << '\n';
                    }, globalStack.top());
                }
                return;
        }
        
        //Increment instruction index at the end
        ++globalInstructionIndex;
    }
}

void ByteCodeInterpreter::interpret()
{
    //Initialize global symbol table with one layer / global layer
    globalSymbolTable.push_back({});

    //Decode and execute instructions
    decodeFile();
    interpretInstructions();
}

//-----------------Helper Fuctions-----------------
std::string& ByteCodeInterpreter::readStringFromFile()
{
    //Get length
    std::size_t length;
    inFile.read(reinterpret_cast<char*>(&length), sizeof(length));

    currentVariable.resize(length);
    inFile.read(&currentVariable[0], length);

    return currentVariable;
}

template<typename T, typename U>
void ByteCodeInterpreter::compare(const T& arg1, const U& arg2, ILInstruction inst)
{
    int result;
    switch (inst) {
        //Comparision operators
        case ILInstruction::CMP_EQ:
            result = (arg1 == arg2);
            break;
        case ILInstruction::CMP_NEQ:
            result = (arg1 != arg2);
            break;
        case ILInstruction::CMP_LT:
            result = (arg1 < arg2);
            break;
        case ILInstruction::CMP_GT:
            result = (arg1 > arg2);
            break;
        case ILInstruction::CMP_LTEQ:
            result = (arg1 <= arg2);
            break;
        case ILInstruction::CMP_GTEQ:
            result = (arg1 >= arg2);
            break;
        //Logical operations
        case ILInstruction::AND:
            result = (arg1 && arg2);
            break;
        case ILInstruction::OR:
            result = (arg1 || arg2);
            break;
        //For not, we only have arg1
        case ILInstruction::NOT:
            result = !arg1;
            break;
    }
    globalStack.push(result);
}

void ByteCodeInterpreter::pushInstructionValue(const InstructionValue& val)
{
    std::visit([&](auto&& arg){
        using T = std::decay_t<decltype(arg)>;

        if constexpr(std::is_same_v<T, int> || std::is_same_v<T, float>)
            globalStack.push(Object(arg));
    }, val);
}

//Symbol table related
void ByteCodeInterpreter::createSymbolTable()
{
    globalSymbolTable.push_back({});
}

void ByteCodeInterpreter::destroySymbolTable()
{
    globalSymbolTable.pop_back();   
}

Object ByteCodeInterpreter::getValueFromSymbolTable(const std::string& id)
{
    for (auto it = globalSymbolTable.rbegin(); it != globalSymbolTable.rend(); ++it) {
        if (it->count(id)) {
            return (*it)[id];
        }
    }
}

void ByteCodeInterpreter::setValueToSymbolTable(const std::string& id, Object elem)
{
    globalSymbolTable.back()[id] = elem;
}