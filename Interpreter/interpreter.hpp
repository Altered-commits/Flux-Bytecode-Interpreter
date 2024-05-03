#ifndef UNNAMED_INTEPRETER_HPP
#define UNNAMED_INTEPRETER_HPP

#include <fstream>
#include <iostream>
#include <cmath>
#include <variant>
#include <vector>

#include "..\Common\InstructionSet.hpp"

using Byte = char;
using Object = std::variant<int, float>; //Had no other name
using InstructionValue = std::variant<int, float, std::size_t, std::string>;

//Storing commands from file
struct Instruction
{
    InstructionValue value;
    ILInstruction    inst;

    Instruction(ILInstruction inst, InstructionValue&& value = 0)
        : inst(inst), value(std::move(value))
    {}
};

class ByteCodeInterpreter {
    public:
        ByteCodeInterpreter(const char* fileName) {
            inFile.open(fileName, std::ios_base::binary);
            
            if (!inFile.is_open()) {
                std::cerr << "[FileReadingError] Error opening file: " << fileName << std::endl;
                std::exit(1);
            }
        }

        void decodeFile();
        void interpretInstructions();
        void interpret();
        void handleUnaryArithmetic();
        void handleIntegerArithmetic(ILInstruction);
        void handleFloatingArithmetic(ILInstruction);
        void handleVariableAssignment(ILInstruction, const std::string&);
        void handleVariableAccess(const std::string&);
        void handleCasting(ILInstruction);
        void handleComparisionAndLogical(ILInstruction);

        //Jump
        void handleJumpIfFalse(std::size_t);
        void handleJump(std::size_t);
    
    private:
        std::string& readStringFromFile();
        
        template<typename T, typename U>
        void compare(const T&, const U&, ILInstruction);

        void pushInstructionValue(const InstructionValue&);
    private:
        std::vector<Instruction> instructions;
        std::string   currentVariable;
        std::ifstream inFile;
};
#endif