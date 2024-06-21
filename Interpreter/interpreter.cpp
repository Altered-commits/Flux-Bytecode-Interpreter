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
std::vector<IterPtr> globalIteratorStack;

//Emulating register (sort of) for return value
Object returnRegister;

//Think of this as program counter
std::uint64_t globalInstructionIndex = 0;

void ByteCodeInterpreter::handleArithmeticOperators(ILInstruction inst)
{
    auto elem1 = globalStack.back();
    globalStack.pop_back();
    auto& elem2 = globalStack.back();

    std::visit([&](auto&& arg1, auto&& arg2) {
        using Elem1Type = std::decay_t<decltype(arg1)>;
        using Elem2Type = std::decay_t<decltype(arg2)>;

        switch (inst)
        {
            case ILInstruction::ADD:
                elem2 = arg2 + arg1; break;
            case ILInstruction::SUB:
                elem2 = arg2 - arg1; break;
            case ILInstruction::MUL:
                elem2 = arg2 * arg1; break;
            case ILInstruction::POW:
                elem2 = std::pow(arg2, arg1); break;
            case ILInstruction::MOD:
            {
                if(arg1 == 0) {
                    std::cout << "[RuntimeError]: Modulus By 0"; std::exit(1);
                }
                if constexpr(std::is_same_v<Elem1Type, int> && std::is_same_v<Elem2Type, int>)
                    elem2 = arg2 % arg1;
                else
                    elem2 = std::fmod(arg2, arg1);
            }
            break;
            case ILInstruction::DIV:
            {
                if(arg1 == 0) {
                    std::cout << "[RuntimeError]: Division By 0"; std::exit(1);
                }
                elem2 = arg2 / arg1;
            }
        }
    }, elem1, elem2);
}

void ByteCodeInterpreter::handleUnaryOperators()
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

    //Pop the stack to get the evaluated expression if necessary
    if (shouldPop)
        globalStack.pop_back();
    
    //Determine whether to reassign the variable or not
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

        inst == CAST_INT ? globalStack.emplace_back(static_cast<std::int64_t>(arg))
                         : globalStack.emplace_back(static_cast<std::double_t>(arg));
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

void ByteCodeInterpreter::handleIteratorInit(const std::string& iterId, std::uint16_t iterParams)
{
    //iterParams -> Iterator Type << 8 | Identifier Type
    //Higher 8bits are Iterator Type
    std::uint8_t iterType  = (iterParams & 0xFF00) >> 8;
    //Lower 8buts are Identifier Type
    std::uint8_t identType = (iterParams & 0x00FF);

    //Get iterator of that type
    switch ((EvalType)identType)
    {
        //Doesn't matter, Auto type is emitted only in Ellipsis Iterator, hence it's of no use. Templates dont matter
        case EVAL_AUTO:
        case EVAL_INT:
            globalIteratorStack.emplace_back(
                getIterator<std::int64_t>(iterId, (IteratorType)iterType)
            );
            break;

        case EVAL_FLOAT:
            globalIteratorStack.emplace_back(
                getIterator<std::double_t>(iterId, (IteratorType)iterType)
            );
            break;
    }
}

void ByteCodeInterpreter::handleIteratorHasNext(std::size_t jumpOffset)
{
    //Get the currently used iterator and call hasNext()
    //If the next element exists, i mean cool, dont do anything, else jump and pop the iterator from stack
    if(!globalIteratorStack.back()->hasNext())
    {
        //Set jump location
        globalInstructionIndex = jumpOffset - 1;
        //Pop the iterator
        globalIteratorStack.pop_back();
    }
}

void ByteCodeInterpreter::handleIteratorNext(std::size_t jumpOffset)
{
    //Call next on iterator
    globalIteratorStack.back()->next();
    //Unconditional jump
    globalInstructionIndex = jumpOffset - 1;
}

void ByteCodeInterpreter::handleReturn(std::size_t returnParams)
{
    constexpr std::size_t numBitsToShift = (sizeof(std::size_t) * CHAR_BIT) - 1;
    constexpr std::size_t topBit         = (std::size_t)1 << numBitsToShift;
    constexpr std::size_t allBitsExceptTopBit = ~topBit;
    
    std::size_t shouldReturn = returnParams & topBit;
    std::size_t returnIndex  = returnParams & allBitsExceptTopBit;

    //We can return
    if(shouldReturn) {
        returnRegister = std::move(globalStack.back());
        globalStack.pop_back();
    }
    //Jump to FUNC_END and let it handle the rest
    globalInstructionIndex = returnIndex - 1;
}

void ByteCodeInterpreter::handleFunctionEnd()
{
    //Destroy function scope and set globalInstructionIndex to what it was before function call
    destroyFunctionScope();
    
    //If we use vargs, clean up them as well till return address
    if(!functionStartingStack.empty())
    {
        globalStack.resize(functionStartingStack.back());
        functionStartingStack.pop_back();
    }

    auto val = std::get<std::uint64_t>(globalStack.back());
    globalInstructionIndex = val - 1;

    globalStack.pop_back();
}

void ByteCodeInterpreter::setFile(const char* filename)
{
    inFile.open(filename, std::ios_base::binary);
            
    if (!inFile) {
        std::cerr << "[FileReadingError]: Error opening file: " << filename << '\n';
        std::exit(1);
    }
}

void ByteCodeInterpreter::decodeFile()
{
    bool hasInst = true;
    
    //Inefficient ahhh
    std::vector<std::pair<std::size_t, ListOfInstruction>> functionStack;
    //Think of this as a main function
    functionStack.emplace_back(0, ListOfInstruction{});
    //Using this so i can change where i can emplace instructions
    ListOfInstruction* refToInstructionList = &(functionStack.back().second);

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
                refToInstructionList->emplace_back(inst);
                hasInst = false;
                break;
            
            case FUNC_START:
            {   
                std::size_t funcStartIndex = readOperand<std::size_t>(chunkBuffer, chunkBufferIndex);
                functionStack.emplace_back(funcStartIndex, ListOfInstruction{});
                refToInstructionList = &(functionStack.back().second);
            }
            break;

            case FUNC_END:
            {
                //Emplace FUNC_END instruction before creating a function frame
                refToInstructionList->emplace_back(ILInstruction::FUNC_END);

                //Create the frame now
                std::size_t funcStartIndex = functionStack.back().first;
                functionTable.emplace(funcStartIndex, std::move(functionStack.back().second));
                functionStack.pop_back();
                refToInstructionList = &(functionStack.back().second);
            }
            break;

            //Primitive types
            case PUSH_INT64:
                refToInstructionList->emplace_back(inst, readOperand<std::int64_t>(chunkBuffer, chunkBufferIndex));
                break;
            case PUSH_UINT64:
                refToInstructionList->emplace_back(inst, readOperand<std::uint64_t>(chunkBuffer, chunkBufferIndex));
                break;
            case PUSH_FLOAT:
                refToInstructionList->emplace_back(inst, readOperand<std::double_t>(chunkBuffer, chunkBufferIndex));
                break;

            //Variable assignment / accessing
            case ILInstruction::ASSIGN_VAR:
            case ILInstruction::ASSIGN_VAR_NO_POP:
                refToInstructionList->emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)), 0);
                break;
            //But for these instruction, these will be followed by a DATAINST_VAR_SCOPE_IDX instruction containing the index
            case ILInstruction::REASSIGN_VAR:
            case ILInstruction::REASSIGN_VAR_NO_POP:
            case ILInstruction::ACCESS_VAR:
                refToInstructionList->emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)),
                                        (*refToInstructionList)[refToInstructionList->size() - 1].scopeIndexIfNeeded);
                break;
            case ILInstruction::DATAINST_VAR_SCOPE_IDX:
                refToInstructionList->emplace_back(inst, 0, readOperand<std::uint16_t>(chunkBuffer, chunkBufferIndex));
                break;
            
            //Jump cases
            case ILInstruction::JUMP:
            case ILInstruction::JUMP_IF_FALSE:
            //As they have same operands to decode, just put them here
            case ILInstruction::ITER_HAS_NEXT:
            case ILInstruction::ITER_NEXT:
            case ILInstruction::FUNC_CALL:
            case ILInstruction::FUNC_VARGS:
            case ILInstruction::RETURN:
                refToInstructionList->emplace_back(inst, readOperand<std::size_t>(chunkBuffer, chunkBufferIndex));
                break;
            
            //Iterator
            case ILInstruction::DATAINST_ITER_ID:
                refToInstructionList->emplace_back(inst, std::move(readStringOperand(chunkBuffer, chunkBufferIndex)));
                break;
            case ILInstruction::ITER_INIT:
            //Same for this instruction
            case ILInstruction::DESTROY_MULTIPLE_SYMBOL_TABLES:
                refToInstructionList->emplace_back(inst, readOperand<std::uint16_t>(chunkBuffer, chunkBufferIndex));
                break;
            //Rest of it just read instruction
            default:
                refToInstructionList->emplace_back(inst);
                break;
        }
    }

    //Move the so called main function scope thingy to 'instructions' as its the first thing used by interpretInstructions
    instructions = std::move(functionStack.back().second);
}

void ByteCodeInterpreter::interpretInstructions(ListOfInstruction& externalInstructions)
{
    if(currentRecursionDepth > maxRecursionDepth)
        printRuntimeError("RecursionError", "Max recursion depth reached, over 1000 function calls");

    while(true)
    {
        Instruction& i = externalInstructions[globalInstructionIndex];

        switch (i.inst)
        {
            case ILInstruction::PUSH_INT64:
            case ILInstruction::PUSH_UINT64:
            case ILInstruction::PUSH_FLOAT:
                pushInstructionValue(i.value);
                break;
            
            //Unary Operations, '+' as unary -> useless ahh
            case ILInstruction::NEG:
                handleUnaryOperators();
                break;
            
            //Arithmetic Operations, idk if this is even the right way
            case ILInstruction::ADD:
            case ILInstruction::SUB:
            case ILInstruction::MUL:
            case ILInstruction::DIV:
            case ILInstruction::MOD:
            case ILInstruction::POW:
                handleArithmeticOperators(i.inst);
                break;
            
            //Casting stuff
            case ILInstruction::CAST_FLOAT:
            case ILInstruction::CAST_INT:
                handleCasting(i.inst);
                break;
            
            //Comparision Operations
            case ILInstruction::CMP_EQ:
            case ILInstruction::CMP_NEQ:
            case ILInstruction::CMP_LT:
            case ILInstruction::CMP_GT:
            case ILInstruction::CMP_LTEQ:
            case ILInstruction::CMP_GTEQ:
            case ILInstruction::CMP_IS:
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
                handleIteratorInit(std::get<std::string>(externalInstructions[globalInstructionIndex - 1].value),
                                    std::get<std::uint16_t>(i.value));
                break;
            case ILInstruction::ITER_HAS_NEXT:
                handleIteratorHasNext(std::get<std::size_t>(i.value));
                break;
            case ILInstruction::ITER_CURRENT:
                setValueToTopFrame(globalIteratorStack.back()->getId(), std::move(globalIteratorStack.back()->getCurrent()));
                break;
            case ILInstruction::ITER_NEXT:
                handleIteratorNext(std::get<std::size_t>(i.value));
                break;
            case ILInstruction::ITER_RECALC_STEP:
                globalIteratorStack.back()->recalcStep();
                break;
            
            //Symbol table
            case ILInstruction::CREATE_SYMBOL_TABLE:
                createSymbolTable();
                break;
            case ILInstruction::DESTROY_SYMBOL_TABLE:
                destroySymbolTable();
                break;
            case ILInstruction::DESTROY_MULTIPLE_SYMBOL_TABLES:
                destroyMultipleSymbolTables(std::get<std::uint16_t>(i.value));
                break;
            
            //Functions and return values
            //Save current stack size and push vargs size as well
            case ILInstruction::FUNC_VARGS:
            {
                functionStartingStack.emplace_back(globalStack.size());
                globalStack.emplace_back(std::get<std::size_t>(i.value));
            }
            break;
            //Return address is already saved to stack, call function resetting instruction index to 0
            case ILInstruction::FUNC_CALL:
            {
                globalInstructionIndex = 0;
                IN_FUNC
                ByteCodeInterpreter::getInstance().interpretInstructions(functionTable.at(std::get<std::size_t>(i.value) - 1));
                OUT_FUNC
            }
                break;
            case ILInstruction::FUNC_END:
                handleFunctionEnd();
                return;
            //Place the value in returnRegister
            case RETURN:
                handleReturn(std::get<std::size_t>(i.value));
                break;
            //Will optimize this later
            case USE_RETURN_VAL:
                globalStack.emplace_back(std::move(returnRegister));
                break;

            //EOFFFFFFFFFFFFF
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
    interpretInstructions(instructions);

    auto end_ii = std::chrono::high_resolution_clock::now();

    std::cout << "Time to decode entire file: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end_df - start_df)).count() << " microsec" << '\n';
    std::cout << "Time to interpret decoded instructions: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end_ii - start_ii)).count() <<  " microsec" << '\n';
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
            auto vstep  = globalStack.back();
            auto vstop  = STACK_REVERSE_ACCESS_ELEM(2);
            auto vstart = STACK_REVERSE_ACCESS_ELEM(3);
            
            globalStack.resize(globalStack.size() - 3);
            
            return std::visit([&](auto&& step, auto&& stop, auto&& start) {
                return std::make_unique<RangeIterator<T>>(id, start, stop, step);
            }, vstep, vstop, vstart);
        }
        break;
        case ELLIPSIS_ITERATOR:
        {
            auto start = functionStartingStack.back();
            //We need to get the size of vargs, which is exactly after top function ret addr
            return std::visit([&](auto&& size) {
                return std::make_unique<EllipsisIterator>(id, start + 1, size);
            }, globalStack[start]);
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
        //Type comparison (is operator)
        case ILInstruction::CMP_IS:
            switch (static_cast<std::uint8_t>(arg2))
            {
                case EVAL_VOID:
                    result = 0; //Void doesn't exist in variable declaration so using 'is' for Void type is always false
                    break;
                case EVAL_INT:
                    result = std::is_same<T, std::int64_t>::value;
                    break;
                case EVAL_FLOAT:
                    result = std::is_same<T, std::double_t>::value;
                    break;
                default:
                    printRuntimeError("ComparisionError", "Unknown type found for 'is' operator");
            }
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

        if constexpr(std::is_same_v<T, std::int64_t>
                    || std::is_same_v<T, std::uint64_t>
                    || std::is_same_v<T, std::double_t>)
            globalStack.emplace_back(std::move(arg));
    }, val);
}

//Symbol table related
void ByteCodeInterpreter::createSymbolTable()
{
    ++currentScope;
    globalSymbolTable.emplace_back();
}

void ByteCodeInterpreter::destroySymbolTable()
{
    --currentScope;
    globalSymbolTable.pop_back();   
}

void ByteCodeInterpreter::destroyMultipleSymbolTables(std::uint16_t scopesToDestroy)
{
    currentScope = std::max(0, currentScope - scopesToDestroy);
    globalSymbolTable.erase(globalSymbolTable.end() - scopesToDestroy, globalSymbolTable.end());
}

void ByteCodeInterpreter::destroyFunctionScope()
{
    std::size_t fallbackToScope = functionStartingScope.back();
    globalSymbolTable.resize(fallbackToScope);
    currentScope = fallbackToScope - 1;
}

const Object& ByteCodeInterpreter::getValueFromNthFrame(const std::string& id, const std::uint8_t scopeIndex)
{
    //Functions use the slower version of symbol table getter thingy cuz,
    if(isFunctionOngoing)
        //If u can't get value from scope index, fuck compiler and fuck you.
        for (std::int32_t i = currentScope; i >= 0; --i)
        {
            auto& table = globalSymbolTable[i];
            auto  value = table.find(id);
            if(value != table.end())
                return value->second;
        }
    else
        return globalSymbolTable[scopeIndex].at(id);
    
    printRuntimeError("WTF?", "How did you even reach to this state, bruh?");
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