#include <unordered_map>
#include <chrono>

#include "interpreter.hpp"

//Just easier to write 
#define STACK_REVERSE_ACCESS_ELEM(n) (globalStack[globalStack.size() - n])

//Global Stack
std::vector<Object> globalStack;

//Global symbol table (stack based scope, using vector)
std::vector<std::unordered_map<std::string, Object>> globalSymbolTable;

//Iterator stack
std::vector<IterPtr> iteratorStack;

//Think of this as program counter
std::size_t globalInstructionIndex = 0;

void ByteCodeInterpreter::handleIntegerArithmetic(ILInstruction inst)
{
    auto& elem1 = std::get<int>(globalStack.back());
    auto& elem2 = std::get<int>(STACK_REVERSE_ACCESS_ELEM(2));

    int out = 0;

    switch (inst)
    {
        case ILInstruction::ADD:
            out = elem2 + elem1; break;
        case ILInstruction::SUB:
            out = elem2 - elem1; break;
        case ILInstruction::MUL:
            out = elem2 * elem1; break;
        case ILInstruction::MOD:
        {
            if(elem1 == 0)
            {
                std::cout << "[RuntimeError]: Modulus By 0"; std::exit(1);
            }
            out = elem2 % elem1;
        }
        break;    
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
    
    // Update the top element in place
    STACK_REVERSE_ACCESS_ELEM(2) = out;
    // Remove the top element
    globalStack.pop_back();
}

void ByteCodeInterpreter::handleFloatingArithmetic(ILInstruction inst)
{
    auto elem1 = 0.0f, elem2 = 0.0f;

    //Doesnt matter if its int or float, we casting it to float always
    //First element to pop
    std::visit([&elem1](auto&& arg){
        elem1 = static_cast<float>(arg);
    }, globalStack.back());

    //Second element to pop
    std::visit([&elem2](auto&& arg){
        elem2 = static_cast<float>(arg);
    }, STACK_REVERSE_ACCESS_ELEM(2));

    float out;

    switch (inst)
    {
        case ILInstruction::FADD:
            out = elem2 + elem1; break;
        case ILInstruction::FSUB:
            out = elem2 - elem1; break;
        case ILInstruction::FMUL:
            out = elem2 * elem1; break;
        case ILInstruction::FMOD:
        {
            if(elem1 == 0.0f)
            {
                std::cout << "[RuntimeError]: Modulus By 0"; std::exit(1);
            }
            out = std::fmod(elem2, elem1);
        }
        break;
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
    
    STACK_REVERSE_ACCESS_ELEM(2) = out;
    globalStack.pop_back();
}

void ByteCodeInterpreter::handleUnaryArithmetic()
{
    //Unary not handled in 'handleComparisionAndLogical'
    std::visit([](auto&& arg) {
        arg = -arg;
    }, globalStack.back());
}

void ByteCodeInterpreter::handleVariableAssignment(ILInstruction inst, const std::string& identifier, const std::uint8_t scopeIndex)
{
    auto elem = globalStack.back();
    
    const bool shouldPop = inst != ASSIGN_VAR_NO_POP && inst != REASSIGN_VAR_NO_POP;
    const bool shouldReassign = inst == REASSIGN_VAR || inst == REASSIGN_VAR_NO_POP;

    // Pop the stack to get the evaluated expression if necessary
    if (shouldPop)
        globalStack.pop_back();
    
    // Determine whether to reassign the variable or not
    shouldReassign ? setValueToNthFrame(identifier, std::move(elem), scopeIndex)
                   : setValueToTopFrame(identifier, std::move(elem));
}

void ByteCodeInterpreter::handleVariableAccess(const std::string& identifier, const std::uint8_t scopeIndex)
{
    //Variant should handle the rest ig
    globalStack.emplace_back(std::move(getValueFromNthFrame(identifier, scopeIndex)));
}

void ByteCodeInterpreter::handleCasting(ILInstruction inst)
{   
    std::visit([&inst](auto&& arg){
        globalStack.pop_back();

        inst == CAST_INT ? globalStack.emplace_back(static_cast<int>(arg))
                         : globalStack.emplace_back(static_cast<float>(arg));
    }, globalStack.back());
}

void ByteCodeInterpreter::handleComparisionAndLogical(ILInstruction inst)
{
    auto elem1 = globalStack.back();
    globalStack.pop_back();
    switch (inst)
    {
        case NOT:
            std::visit([&](auto&& arg1){
                compare(arg1, 0, inst);
            }, elem1);
            break;
        //Rest of the comparision / logical stuff
        default:
            auto elem2 = globalStack.back();
            globalStack.pop_back();

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
        globalStack.pop_back();
        //Check the condition, if arg is false, we jump, aka change the globalIndex
        globalInstructionIndex = !arg ? jumpOffset - 1 : globalInstructionIndex;
        //or we just dont do anything
    }, globalStack.back());
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

    //Our buffer which will store chunks of file
    std::size_t       chunkBufferIndex;
    std::array<Byte, FILE_READ_CHUNK_SIZE> chunkBuffer;

    //Read one chunk initially ofc
    readFileChunk(chunkBuffer, chunkBufferIndex);

    while (hasInst)
    {
        //Instruction is 1 byte, read it and increment
        Byte instByte = chunkBuffer[chunkBufferIndex++];
        ILInstruction inst = static_cast<ILInstruction>(instByte);

        //Check if we need to refill buffer
        if(chunkBufferIndex >= chunkBuffer.size())
            readFileChunk(chunkBuffer, chunkBufferIndex);

        switch (inst)
        {
            case END_OF_FILE:
                instructions.emplace_back(inst);
                hasInst = false;
                break;
            //Primitive types
            case PUSH_INT:
                //Read operand as int
                instructions.emplace_back(inst, readOperand<int>(chunkBuffer, chunkBufferIndex));
                break;
            case PUSH_FLOAT:
                //Read operand as float
                instructions.emplace_back(inst, readOperand<float>(chunkBuffer, chunkBufferIndex));
                break;

            //Variable assignment / accessing
            case ILInstruction::ASSIGN_VAR:
            case ILInstruction::ASSIGN_VAR_NO_POP:
                instructions.emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)), 0);
                break;
            //But for these instruction, these will be followed by a DATAINST_VAR_SCOPE_IDX instruction containing the index
            case ILInstruction::REASSIGN_VAR:
            case ILInstruction::REASSIGN_VAR_NO_POP:
            case ILInstruction::ACCESS_VAR:
                instructions.emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)),
                                        instructions[instructions.size() - 1].scopeIndexIfNeeded);
                break;
            case ILInstruction::DATAINST_VAR_SCOPE_IDX:
                instructions.emplace_back(inst, 0, readOperand<std::uint16_t>(chunkBuffer, chunkBufferIndex));
                break;
            
            //Jump cases
            case ILInstruction::JUMP:
            case ILInstruction::JUMP_IF_FALSE:
            //As they have same operands to decode, just put them here
            case ILInstruction::ITER_HAS_NEXT:
            case ILInstruction::ITER_NEXT:
                instructions.emplace_back(inst, readOperand<std::size_t>(chunkBuffer, chunkBufferIndex));
                break;
            
            //Iterator
            case ILInstruction::DATAINST_ITER_ID:
                instructions.emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)));
                break;
            case ILInstruction::ITER_INIT:
            //Same for this instruction
            case ILInstruction::DESTROY_MULTIPLE_SYM_TABLES:
                instructions.emplace_back(inst, readOperand<std::uint16_t>(chunkBuffer, chunkBufferIndex));
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
            case ILInstruction::MOD:
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
            case ILInstruction::FMOD:
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
                handleVariableAssignment(i.inst, std::get<std::string>(i.value), i.scopeIndexIfNeeded);
                break;
            
            case ILInstruction::ACCESS_VAR:
                handleVariableAccess(std::get<std::string>(i.value), i.scopeIndexIfNeeded);
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
            
            case ILInstruction::DESTROY_MULTIPLE_SYM_TABLES:
                destroyMultipleSymbolTables(std::get<std::uint16_t>(i.value));
                break;

            case END_OF_FILE:
                std::cout << "Successfully Interpreted, Read all symbols.\n";

                if(!globalStack.empty())
                {
                    std::visit([](auto&& arg){
                        std::cout << "Top of stack: " << arg << '\n';
                    }, globalStack.back());
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
    globalSymbolTable.emplace_back();

    auto start_df = std::chrono::high_resolution_clock::now();

    //Decode File
    decodeFile();

    auto end_df = std::chrono::high_resolution_clock::now();
    
    auto start_ii = std::chrono::high_resolution_clock::now();

    //Execute instructions
    interpretInstructions();

    auto end_ii = std::chrono::high_resolution_clock::now();

    std::cout << "Time to decode entire file: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end_df - start_df)).count() << "ms" << '\n';
    std::cout << "Time to interpret decoded instructions: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end_ii - start_ii)).count() << "ms" << '\n';
}

//-----------------Helper Fuctions-----------------
template<typename T>
IterPtr ByteCodeInterpreter::getIterator(const std::string& id, IteratorType iterType)
{
    switch (iterType)
    {
        //Defined in common.hpp
        case RANGE_ITERATOR:
        {
            //Pop three values from stack (step, stop, start)
            auto vstep  = STACK_REVERSE_ACCESS_ELEM(1);
            auto vstop  = STACK_REVERSE_ACCESS_ELEM(2);
            auto vstart = STACK_REVERSE_ACCESS_ELEM(3);
            
            globalStack.resize(globalStack.size() - 3);
            
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
    int result = 0;
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
    globalStack.push_back(result);
}

void ByteCodeInterpreter::pushInstructionValue(const InstructionValue& val)
{
    std::visit([&](auto&& arg){
        using T = std::decay_t<decltype(arg)>;

        if constexpr(std::is_same_v<T, int> || std::is_same_v<T, float>)
            globalStack.emplace_back(std::move(arg));
    }, val);
}

//Symbol table related
void ByteCodeInterpreter::createSymbolTable()
{
    globalSymbolTable.emplace_back();
}

void ByteCodeInterpreter::destroySymbolTable()
{
    globalSymbolTable.pop_back();   
}

void ByteCodeInterpreter::destroyMultipleSymbolTables(std::uint16_t scopesToDestroy)
{
    for(int i = 0; i < scopesToDestroy; i++)
        globalSymbolTable.pop_back();
}

const Object& ByteCodeInterpreter::getValueFromNthFrame(const std::string& id, const std::uint8_t scopeIndex)
{
    return globalSymbolTable[scopeIndex].at(id);
}

void ByteCodeInterpreter::setValueToTopFrame(const std::string& id, Object&& elem)
{
    globalSymbolTable.back()[id] = std::move(elem);
}

void ByteCodeInterpreter::setValueToNthFrame(const std::string& id, Object&& elem, const std::uint8_t scopeIndex)
{
    globalSymbolTable[scopeIndex][id] = std::move(elem);
}

//File decoding related
void ByteCodeInterpreter::readFileChunk(std::array<Byte, FILE_READ_CHUNK_SIZE>& chunkBuffer, std::size_t& chunkBufferIndex)
{
    inFile.read(chunkBuffer.data(), FILE_READ_CHUNK_SIZE);
    chunkBufferIndex = 0;
}

template<typename T>
T ByteCodeInterpreter::readOperand(std::array<Byte, FILE_READ_CHUNK_SIZE>& chunkBuffer, std::size_t& chunkBufferIndex)
{
    //IF rest of the stuff is outside of current chunk, handle it
    if(chunkBufferIndex + sizeof(T) > chunkBuffer.size())
    {
        //Read the remaining bytes first
        const std::size_t remainingBytesInCurrentChunk = (chunkBuffer.size() - chunkBufferIndex);
        //This many no. of bytes is to be read in the next chunk
        const std::size_t remainingBytesToRead         = sizeof(T) - remainingBytesInCurrentChunk;
        //Value to be read to
        T splitValue;
        
        ///STEP1: Read the remaining bytes in current chunk first into the value
        std::memcpy(&splitValue, chunkBuffer.data() + chunkBufferIndex, remainingBytesInCurrentChunk);

        ///STEP 2: Read the next chunk of file
        readFileChunk(chunkBuffer, chunkBufferIndex);

        ///STEP 3: Read the remaining bytes residing in this chunk now, from where we left of
        std::memcpy(reinterpret_cast<char*>(&splitValue) + remainingBytesInCurrentChunk, chunkBuffer.data(), remainingBytesToRead);
        
        ///STEP 4: Increment the index
        chunkBufferIndex += remainingBytesToRead;

        //Finally return it
        return splitValue;
    }

    //Read the value from current index
    T value;
    std::memcpy(&value, chunkBuffer.data() + chunkBufferIndex, sizeof(T));
    chunkBufferIndex += sizeof(T);

    return value;
}

std::string& ByteCodeInterpreter::readStringOperand(std::array<Byte, FILE_READ_CHUNK_SIZE>& chunkBuffer, std::size_t& chunkBufferIndex)
{
    //Read length of string first
    std::size_t strLength = readOperand<std::size_t>(chunkBuffer, chunkBufferIndex);

    //Resize storage variable
    currentVariable.resize(strLength);
    
    ///Same as readOperand, copied the entire thing
    if(chunkBufferIndex + strLength > chunkBuffer.size())
    {
        const std::size_t remainingBytesInCurrentChunk = (chunkBuffer.size() - chunkBufferIndex);
        const std::size_t remainingBytesToRead         = strLength - remainingBytesInCurrentChunk;

        std::memcpy(currentVariable.data(), chunkBuffer.data() + chunkBufferIndex, remainingBytesInCurrentChunk);
        readFileChunk(chunkBuffer, chunkBufferIndex);
        std::memcpy(currentVariable.data() + remainingBytesInCurrentChunk, chunkBuffer.data(), remainingBytesToRead);
        
        chunkBufferIndex += remainingBytesToRead;
        return currentVariable;
    }
    //Else its inside the chunk
    std::memcpy(currentVariable.data(), chunkBuffer.data() + chunkBufferIndex, strLength);
    chunkBufferIndex += strLength;

    return currentVariable;
}