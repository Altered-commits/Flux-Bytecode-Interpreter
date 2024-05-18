#ifndef UNNAMED_ERROR_PRINTER_HPP
#define UNNAMED_ERROR_PRINTER_HPP

#include <string>
#include <cstdint>
#include <iostream>

#include "lexer.hpp"
#include "..\Common\common.hpp" //EvalType

static void printError(std::string errorSection, std::string errorMsg)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();
    std::cout << "[" << errorSection << "]: "
                << errorMsg << '\n'
                << "[Error info]: Error came from -> Line: " << cur_line << ", Column: " << cur_col << '\n';
    std::exit(1);
}

template<typename... Args>
static void printError(std::string errorSection, const std::string& errorMsg, Args&&... args)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();
    std::cout << "[" << errorSection << "]: "
              << errorMsg;
    
    // Output additional arguments like std::cout
    ((std::cout << args), ...);
    
    std::cout << '\n'
              << "[Error info]: Error came from -> Line: " << cur_line << ", Column: " << cur_col << '\n';
    std::exit(1);
}

//For ILGen errors specifically, normally there shouldnt be a single error in this
static void printError(std::string errorMsg, EvalType type)
{
    std::cout << "[ILGeneratorError]: " << errorMsg << (int)type << '\n';
    std::exit(1);
}
static void printError(std::string errorMsg, TokenType type)
{
    std::cout << "[ILGeneratorError]: " << errorMsg << (int)type << '\n';
    std::exit(1);
}

//I might as well have this print warnings as well ig
static void printWarning(std::string warningSection, const std::string& warningMsg)
{
    auto [cur_line, cur_col] = Lexer::getLineColCount();

    std::cout << "[Warning] From [" <<  warningSection << "]: " << warningMsg
                << "\n[Warning Info]: " << "Line: " << cur_line << ", Column: " << cur_col << '\n';
}
#endif