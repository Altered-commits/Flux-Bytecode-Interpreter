#include <stack>
#include <unordered_map>

#include "interpreter.hpp"

//Global Stack
std::stack<Object> globalStack;

//Global symbol table (stack based scope, using vector)
std::vector<std::unordered_map<std::string, Object>> globalSymbolTable;

//Iterator stack
std::vector<IterPtr> iteratorStack;

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

    switch (inst)
    {
        //If assign var: pop and assign, Else: assign
        case ASSIGN_VAR:
            globalStack.pop();
        case ASSIGN_VAR_NO_POP:
            setValueToTopFrame(identifier, elem);
            break;

        //Only difference, look the entire symbol table to set value
        case REASSIGN_VAR:
            globalStack.pop();
        case REASSIGN_VAR_NO_POP:
            setValueToNthFrame(identifier, elem);
            break;
    }
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

void ByteCodeInterpreter::handleIteratorInit(std::uint16_t iterParams)
{
    //Get identifier from previous instruction
    //PreInit consists of the identifier
    std::string iterId = std::get<std::string>(instructions[globalInstructionIndex - 1].value);

    //iterParams -> Iterator Type << 8 | Identifier Type
    //Higher 8bits are Iterator Type
    std::uint8_t iterType  = (iterParams & 0xFF00) >> 8;
    //Lower 8buts are Identifier Type
    std::uint8_t identType = (iterParams & 0x00FF);

    //Get iterator of that type
    switch (identType)
    {
        //TOKEN_INT -> 0
        case 0:
            iteratorStack.emplace_back(
                getIterator<int>(iterId, (IteratorType)iterType)
            );
            break;

        //TOKEN_FLOAT -> 1
        case 1:
            iteratorStack.emplace_back(
                getIterator<float>(iterId, (IteratorType)iterType)
            );
            break;
    }
}

void ByteCodeInterpreter::handleIteratorHasNext(std::size_t jumpOffset)
{
    //Get the currently used iterator and call hasNext()
    //If the next element exists, i mean cool, dont do anything, else jump and pop the iterator from stack
    if(!iteratorStack.back()->hasNext())
    {
        //Set jump location
        globalInstructionIndex = jumpOffset - 1;
        //Pop the iterator
        iteratorStack.pop_back();
    }
}

void ByteCodeInterpreter::handleIteratorNext(std::size_t jumpOffset)
{
    //Call next on iterator
    iteratorStack.back()->next();
    //Unconditional jump
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
            case ILInstruction::REASSIGN_VAR:
            case ILInstruction::REASSIGN_VAR_NO_POP:
            case ILInstruction::ACCESS_VAR:
                instructions.emplace_back(inst, std::move(readStringFromFile()));
                break;
            
            //Jump cases
            case ILInstruction::JUMP:
            case ILInstruction::JUMP_IF_FALSE:
            //As they have same operands to decode, just put them here
            case ILInstruction::ITER_HAS_NEXT:
            case ILInstruction::ITER_NEXT:
            {
                std::size_t jumpOffset;
                inFile.read(reinterpret_cast<Byte*>(&jumpOffset), sizeof(std::size_t));
                instructions.emplace_back(inst, std::move(jumpOffset));
            }
            break;
            
            //Iterator
            case ILInstruction::ITER_PRE_INIT:
                instructions.emplace_back(inst, std::move(readStringFromFile()));
                break;
            case ILInstruction::ITER_INIT:
            {
                std::uint16_t iterParams;
                inFile.read(reinterpret_cast<Byte*>(&iterParams), sizeof(std::uint16_t));
                instructions.emplace_back(inst, std::move(iterParams));
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
            
            //Casting stuff
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
            case ILInstruction::REASSIGN_VAR:
            case ILInstruction::REASSIGN_VAR_NO_POP:
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
            
            //Iterators
            case ILInstruction::ITER_PRE_INIT:
                setValueToTopFrame(std::get<std::string>(i.value), 0);
                break;
            case ILInstruction::ITER_INIT:
                handleIteratorInit(std::get<std::uint16_t>(i.value));
                break;
            case ILInstruction::ITER_HAS_NEXT:
                handleIteratorHasNext(std::get<std::size_t>(i.value));
                break;
            case ILInstruction::ITER_CURRENT:
                setValueToTopFrame(iteratorStack.back()->getId(), std::move(iteratorStack.back()->getCurrent()));
                break;
            case ILInstruction::ITER_NEXT:
                handleIteratorNext(std::get<std::size_t>(i.value));
                break;
            case ILInstruction::ITER_RECALC_STEP:
                iteratorStack.back()->recalcStep();
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
    //Initialize global symbol table with one frame / global frame
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

template<typename T>
IterPtr ByteCodeInterpreter::getIterator(const std::string& id, IteratorType iterType)
{
    switch (iterType)
    {
        //Defined in common.hpp
        case RANGE_ITERATOR:
        {
            //Pop three values from stack (step, stop, start)
            auto vstep  = globalStack.top(); globalStack.pop();
            auto vstop  = globalStack.top(); globalStack.pop();
            auto vstart = globalStack.top(); globalStack.pop();
            return std::visit([&](auto&& step, auto&& stop, auto&& start){
                //Create the iterator and return it
                return std::make_unique<RangeIterator<T>>(id, start, stop, step);
            }, vstep, vstop, vstart);
        }
        break;
        default:
            std::cout << "Unsupported Iterator\n"; std::exit(1);
    }
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

void ByteCodeInterpreter::setValueToTopFrame(const std::string& id, Object elem)
{
    globalSymbolTable.back()[id] = std::move(elem);
}

void ByteCodeInterpreter::setValueToNthFrame(const std::string& id, Object elem)
{
    for (auto it = globalSymbolTable.rbegin(); it != globalSymbolTable.rend(); ++it) {
        if (it->count(id)) {
            (*it)[id] = std::move(elem);
            break;
        }
    }
}