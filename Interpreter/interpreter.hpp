#ifndef UNNAMED_INTEPRETER_HPP
#define UNNAMED_INTEPRETER_HPP

#include <fstream>
#include <iostream>
#include <cmath>
#include <variant>

#include "..\InstructionSet\instruction_set.hpp"

using Byte = char;
using ValueType = std::variant<int, float>;

class ByteCodeInterpreter {
    public:
        ByteCodeInterpreter(const char* fileName) {
            inFile.open(fileName, std::ios_base::binary);
            
            if (!inFile.is_open()) {
                std::cerr << "[FileReadingError] Error opening file: " << fileName << std::endl;
                std::exit(1);
            }
        }

        void readFromFile();
        void handleUnaryArithmetic();
        void handleIntegerArithmetic(ILInstruction);
        void handleFloatingArithmetic(ILInstruction);
        void handleVariableAssignment(ILInstruction);
        void handleVariableAccess();
        void handleCasting(ILInstruction);
        void handleComparisionAndLogical(ILInstruction);
    
    private:
        std::string& readStringFromFile();
        
        template<typename T, typename U>
        void compare(const T&, const U&, ILInstruction);
    private:
        std::string   currentVariable;
        std::ifstream inFile;
};

#endif