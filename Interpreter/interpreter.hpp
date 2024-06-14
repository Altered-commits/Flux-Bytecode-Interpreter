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
#include <unordered_map>

#include "..\Common\common.hpp"
//Will later make this common for both compiler and interpreter
#include "..\Common\error_printer.hpp"

using Byte   = char;
using Object = std::variant<std::uint64_t, std::int32_t, std::uint32_t, float, double>; //Had no other name
using FunctionTable = std::unordered_map<std::size_t, ListOfInstruction>;

#include "iterators.hpp"

//Again to be strictly used in ByteCodeInterpreter member functions
#define IN_FUNC bool prevFuncStatus = isFunctionOngoing;\
                     isFunctionOngoing = true;\
                    ++currentRecursionDepth;
#define OUT_FUNC isFunctionOngoing = prevFuncStatus;\
                    --currentRecursionDepth;

class ByteCodeInterpreter {
    private:
        ByteCodeInterpreter() {}

        ByteCodeInterpreter(const ByteCodeInterpreter&) = delete;
        ByteCodeInterpreter(ByteCodeInterpreter&&)      = delete;

    public:
        static ByteCodeInterpreter& getInstance()
        {
            static ByteCodeInterpreter instance;
            return instance;
        }

        void setFile(const char*);
        void interpret();

    private:
        void decodeFile();
        void interpretInstructions(ListOfInstruction&);
    
    private:
        void handleUnaryOperators();
        void handleArithmeticOperators(ILInstruction);
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
        //Function and Return
        void handleReturn(std::size_t);
        void handleFunctionEnd(std::size_t);
    
    //Symbol table related
    private:
        void          createSymbolTable();
        void          destroySymbolTable();
        void          destroyMultipleSymbolTables(std::uint16_t);
        void          destroyFunctionScope(std::uint16_t);
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
        ListOfInstruction instructions;
        std::string       currentVariable;
        std::ifstream     inFile;
        std::int32_t      currentScope = 0;
        //Function stuff
        FunctionTable     functionTable;
        bool              isFunctionOngoing = false;
        std::uint32_t     maxRecursionDepth = 10000, currentRecursionDepth = 0;
};
#endif