#ifndef UNNAMED_INTEPRETER_HPP
#define UNNAMED_INTEPRETER_HPP

#include <fstream>
#include <iostream>
#include <cmath>
#include <cstring>
#include <variant>
#include <vector>
#include <array>
#include <unordered_map>

#include "..\Common\common.hpp"
#include "..\Common\error_printer.hpp"

//Just to simply make the horrendous c++ code look much better
using Byte   = char;
using Object = std::variant<std::uint64_t, std::int64_t, std::double_t>; //Had no other name

#include "iterators.hpp"
//File decoding related
#define FILE_READ_CHUNK_SIZE 2048

//Same for these as well...
using FunctionTable = std::unordered_map<std::size_t, ListOfInstruction>;
using SymbolTable   = std::vector<std::unordered_map<std::string, Object>>;
using IteratorStack = std::vector<IterPtr>;
using ObjectStack   = std::vector<Object>;
using ByteArray     = std::array<Byte, FILE_READ_CHUNK_SIZE>;

//Again to be strictly used in ByteCodeInterpreter member functions
#define IN_FUNC ++currentCallStackDepth;\
                functionStartingScope.emplace_back(globalSymbolTable.size());
#define OUT_FUNC --currentCallStackDepth;\
                 functionStartingScope.pop_back();

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
        void handleIteratorInit(const std::string&, std::uint16_t);
        void handleIteratorHasNext(std::size_t);
        void handleIteratorNext(std::size_t);
        //Function and Return
        void handleReturn(std::size_t);
        void handleFunctionEnd();
    
    //Symbol table related
    private:
        void          createSymbolTable();
        void          destroySymbolTable();
        void          destroyMultipleSymbolTables(std::uint16_t);
        void          destroyFunctionScope();
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
        void readFileChunk(ByteArray&, std::size_t&);
        template<typename T>
        T readOperand(ByteArray&, std::size_t&);
        std::string& readStringOperand(ByteArray&, std::size_t&);
    
    private:
        ListOfInstruction instructions;
        std::string       currentVariable;
        std::ifstream     inFile;
        //Function stuff
        FunctionTable     functionTable;
        std::uint32_t     maxCallStackDepth = 1000, currentCallStackDepth = 0;
        std::vector<std::uint16_t> functionStartingScope = {1}; // Initial (main) scope u could say
        std::vector<std::size_t>   functionStartingStack; //Might merge this and above vector in future
};
#endif