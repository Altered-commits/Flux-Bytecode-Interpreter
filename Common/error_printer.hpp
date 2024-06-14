#ifndef UNNAMED_ERROR_PRINTER_HPP
#define UNNAMED_ERROR_PRINTER_HPP

#include <string>
#include <cstdint>
#include <iostream>

#include "..\Compiler\lexer.hpp"
#include "..\Common\common.hpp" //EvalType

//Works if ur console supports it :pensive:
struct ConsoleTextColors
{
    constexpr static const char* HEADER    = "\033[95m",
                               * OKBLUE    = "\033[94m",
                               * OKCYAN    = "\033[96m",
                               * OKGREEN   = "\033[92m",
                               * WARNING   = "\033[93m",
                               * FAIL      = "\033[91m",
                               * ENDC      = "\033[0m",
                               * BOLD      = "\033[1m",
                               * UNDERLINE = "\033[4m";
};

static void printError(const std::string& errorSection, const std::string& errorMsg)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();
    std::cout << ConsoleTextColors::FAIL
                << "[" << errorSection << "]: "
                << errorMsg
              << ConsoleTextColors::ENDC << '\n'
              
              << ConsoleTextColors::WARNING
                << "[Error info]: Error came from -> Line: " << cur_line << ", Column: " << cur_col
              << ConsoleTextColors::ENDC << '\n';
    std::exit(1);
}

template<typename... Args>
static void printError(const std::string& errorSection, const std::string& errorMsg, Args&&... args)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();
    std::cout << ConsoleTextColors::FAIL
                << "[" << errorSection << "]: "
              << errorMsg;
    
    // Output additional arguments like std::cout
    ((std::cout << args), ...);
    
    std::cout << ConsoleTextColors::ENDC << '\n'

              << ConsoleTextColors::WARNING
                << "[Error info]: Error came from -> Line: " << cur_line << ", Column: " << cur_col
              << ConsoleTextColors::ENDC << '\n';
    std::exit(1);
}

//For ILGen errors specifically, normally there shouldnt be a single error in this
static void printError(const std::string& errorMsg, EvalType type)
{
    std::cout << ConsoleTextColors::FAIL << "[ILGeneratorError]: " << errorMsg << (int)type << ConsoleTextColors::ENDC << '\n';
    std::exit(1);
}
static void printError(const std::string& errorMsg, TokenType type)
{
    std::cout << ConsoleTextColors::FAIL << "[ILGeneratorError]: " << errorMsg << (int)type << ConsoleTextColors::ENDC << '\n';
    std::exit(1);
}

//I might as well have this print warnings as well ig
static void printWarning(const std::string& warningSection, const std::string& warningMsg)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();

    std::cout << ConsoleTextColors::WARNING
                << "[Warning] From [" <<  warningSection << "]: " << warningMsg
                << "\n[Warning Info]: " << "Line: " << cur_line << ", Column: " << cur_col
              << ConsoleTextColors::ENDC << '\n';
}

//RuntimeErrors
static void printRuntimeError(const std::string& errorSection, const std::string& errorMsg)
{
    std::cout << ConsoleTextColors::FAIL
                << "[" << errorSection << "]: "
                << errorMsg
              << ConsoleTextColors::ENDC << '\n';
              
    std::exit(1);
}


#endif