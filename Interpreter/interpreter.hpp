#ifndef UNNAMED_INTEPRETER_HPP
#define UNNAMED_INTEPRETER_HPP

//File decoding related
#define FILE_READ_CHUNK_SIZE 2048

#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <variant>
#include <vector>
#include <array>

#include "..\Common\common.hpp"

using Byte = char;
using Object = std::variant<int, float>; //Had no other name
using InstructionValue = std::variant<int, float, std::uint16_t, std::size_t, std::string>;

#include "iterators.hpp"

//Storing commands from file
struct Instruction
{
    InstructionValue value;
    ILInstruction    inst;

    //Additional data, for scope indexing (for instructions like ACCESS_VAR or ASSIGN_VAR)
    //Idk why im having this as a literal 16bit value but ight
    std::uint16_t scopeIndexIfNeeded;

    Instruction(ILInstruction inst, InstructionValue&& value = 0, std::uint16_t scopeIndex = 0)
        : inst(inst), value(std::move(value)), scopeIndexIfNeeded(scopeIndex)
    {}
};

class ByteCodeInterpreter {
    public:
        ByteCodeInterpreter(const char* fileName) {
            inFile.open(fileName, std::ios_base::binary);
            
            if (!inFile.is_open()) {
                std::cerr << "[FileReadingError]: Error opening file: " << fileName << '\n';
                std::exit(1);
            }

            if (!inFile.good() || inFile.fail()) {
                std::cerr << "[FileReadingError]: File not in good state: " << fileName << '\n';
                std::exit(1);
            }
        }

        void interpret();

    private:
        void decodeFile();
        void interpretInstructions();
    
    private:
        void handleUnaryArithmetic();
        void handleIntegerArithmetic(ILInstruction);
        void handleFloatingArithmetic(ILInstruction);
        void handleVariableAssignment(ILInstruction, const std::string&, const std::uint8_t);
        void handleVariableAccess(const std::string&, const std::uint8_t);
        void handleCasting(ILInstruction);
        void handleComparisionAndLogical(ILInstruction);
        //Jump
        void handleJumpIfFalse(std::size_t);
        void handleJump(std::size_t);
        //Iterator
        void handleIteratorInit(std::uint16_t);
        void handleIteratorHasNext(std::size_t);
        void handleIteratorNext(std::size_t);
    
    //Symbol table related
    private:
        void          createSymbolTable();
        void          destroySymbolTable();
        void          destroyMultipleSymbolTables(std::uint16_t);
        void          setValueToTopFrame(const std::string&, Object&&);
        void          setValueToNthFrame(const std::string&, Object&&, const std::uint8_t);
        const Object& getValueFromNthFrame(const std::string&, const std::uint8_t);
    
    private: //Helper functions
        template<typename T>
        IterPtr getIterator(const std::string&, IteratorType);
        template<typename T, typename U>
        void    compare(const T&, const U&, ILInstruction);
        void    pushInstructionValue(const InstructionValue&);

    //File decoding related
    private:
        void readFileChunk(std::array<Byte, FILE_READ_CHUNK_SIZE>&, std::size_t&);
        template<typename T>
        T readOperand(std::array<Byte, FILE_READ_CHUNK_SIZE>&, std::size_t&);
        std::string& readStringOperand(std::array<Byte, FILE_READ_CHUNK_SIZE>&, std::size_t&);
    
    private:
        std::vector<Instruction> instructions;
        std::string              currentVariable;
        std::ifstream            inFile;
};
#endif