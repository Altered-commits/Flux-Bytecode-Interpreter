#ifndef UNNAMED_ERROR_PRINTER_HPP
#define UNNAMED_ERROR_PRINTER_HPP

#include <string>
#include <cstdint>
#include <iostream>

#include "lexer.hpp"

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

//For ILGen errors specifically
static void printError(std::string errorMsg, TokenType operandType)
{
    std::cout << "[ILGeneratorError]: " << errorMsg << operandType << '\n';
    std::exit(1);
}

#endif