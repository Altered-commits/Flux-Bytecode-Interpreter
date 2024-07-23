#ifndef UNNAMED_BUILTINS_HPP 
#define UNNAMED_BUILTINS_HPP

/*
 * ALL FUNCTIONS WILL BE RETURNING VOID TYPE AND WILL NOT TAKE ANY ARGUMENTS,
 * ANY ARGUMENT OR RETURN VALUE MUST BE DIRECTLY PUSHED OR POPPED BY FUNCTIONS TO STACK
*/

#include <vector>
#include <functional>
#include <unordered_map>

#include "interpreter.hpp" //Object

//Stack is in interpreter.cpp
extern std::vector<Object> globalStack;

//----------ALL THE BUILTINS FROM HERE----------
void __VMInternals_WriteToConsole__();
void __VMInternals_ReadIntFromConsole__();

//----------BUILTIN TABLE----------
//BuiltinType found in Common/common.hpp
static std::unordered_map<BuiltinType, std::function<void()>> builtinTable = {
    {BUILTIN_WRITE_CONSOLE, &__VMInternals_WriteToConsole__    },
    {BUILTIN_IREAD_CONSOLE, &__VMInternals_ReadIntFromConsole__}
};

#endif