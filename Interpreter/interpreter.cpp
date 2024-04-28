#include <stack>
#include <variant>
#include <map>

#include "interpreter.hpp"
//Byte code will be like this
//Instruction Operand 

//Global Stack
std::stack<std::variant<int, float>> globalStack;

//Global symbol table
std::map<std::string, std::variant<int, float>> globalSymbolTable;

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

void ByteCodeInterpreter::handleVariableAssignment(ILInstruction inst)
{
    //After the instruction, read the variable and push it to global symbol table
    std::string& identifier = readStringFromFile();
    
    //But before pushing it to symbol table, pop the stack to get evaluated expression
    auto elem = globalStack.top();

    //Depending on the instruction, we either pop or dont pop
    if(inst != ASSIGN_VAR_NO_POP)
        globalStack.pop();

    // and assign it to variable :D
    globalSymbolTable[identifier] = elem;
}

void ByteCodeInterpreter::handleVariableAccess()
{
    //Read the variable name
    std::string& identifier = readStringFromFile();

    //Variant should handle the rest ig
    globalStack.push(globalSymbolTable.at(identifier));
}

void ByteCodeInterpreter::handleCasting(ILInstruction inst)
{   
    std::visit([&inst](auto&& arg){
        globalStack.pop();

        inst == CAST_INT ? globalStack.push(static_cast<int>(arg))
                         : globalStack.push(static_cast<float>(arg));
    }, globalStack.top());
}

void ByteCodeInterpreter::readFromFile()
{
    bool shouldRead = true;
    while (shouldRead)
    {
        Byte instructionByte;
        inFile.get(instructionByte);
        
        ILInstruction inst = static_cast<ILInstruction>(instructionByte);
        switch (inst)
        {
            case ILInstruction::PUSH_INT:
                {
                    int intValue;
                    inFile.read(reinterpret_cast<Byte*>(&intValue), sizeof(int));
                    
                    globalStack.push(intValue);
                }
                break;
            case ILInstruction::PUSH_FLOAT:
                {
                    float floatValue;
                    inFile.read(reinterpret_cast<Byte*>(&floatValue), sizeof(float));

                    globalStack.push(floatValue);
                }
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
                handleIntegerArithmetic(inst);
                break;
            
            //Casting stuff :D
            case ILInstruction::CAST_FLOAT:
            case ILInstruction::CAST_INT:
                handleCasting(inst);
                break;
            
            //Floating Operations and Power arithmetic, as it also requires both to be floating point
            case ILInstruction::FADD:
            case ILInstruction::FSUB:
            case ILInstruction::FMUL:
            case ILInstruction::FDIV:
            case ILInstruction::POW:
                handleFloatingArithmetic(inst);
                break;
            
            case ILInstruction::ASSIGN_VAR:
            case ILInstruction::ASSIGN_VAR_NO_POP:
                handleVariableAssignment(inst);
                break;
            
            case ILInstruction::ACCESS_VAR:
                handleVariableAccess();
                break;

            //End of interpretation, close it
            case ILInstruction::END_OF_FILE:
                shouldRead = false; break;
        }
    }
    std::cout << "Successfully Interpreted, Read all symbols.\n";

    std::visit([](auto&& arg){
        std::cout << "Top of stack: " << arg << '\n';
    }, globalStack.top());
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