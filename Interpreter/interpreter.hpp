#ifndef UNNAMED_INTEPRETER_HPP
#define UNNAMED_INTEPRETER_HPP

#include <fstream>
#include <iostream>
#include <cmath>

#include "..\InstructionSet\instruction_set.hpp"

using Byte = char;

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
        void handleVariableAssignment();
        void handleVariableAccess();
        void handleCasting(ILInstruction);
    
    private:
        std::string& readStringFromFile();
    private:
        std::string   currentVariable;
        std::ifstream inFile;
};

#endif