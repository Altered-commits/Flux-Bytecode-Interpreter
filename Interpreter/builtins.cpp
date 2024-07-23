#include "builtins.hpp"

void __VMInternals_WriteToConsole__()
{
    //Get the number of arguments (uint64_t)
    auto nArgs = std::get<std::uint64_t>(globalStack.back());
    globalStack.pop_back();

    //No arguments, just print newline
    if(!nArgs) {
        std::cout << '\n';
        return;
    }
    
    //Visit each arg and print it to console
    for (std::size_t i = 0; i < nArgs; i++)
        std::visit([&](auto&& arg) {
            std::cout << arg << '\n';
        }, globalStack[globalStack.size() - i - 1]);

    //Clean the stack
    globalStack.erase(globalStack.end() - nArgs, globalStack.end());
}

void __VMInternals_ReadIntFromConsole__()
{
    //Read a 64bit integer from console
    std::int64_t val;
    std::cin >> val;

    //Push the value to stack
    globalStack.push_back(val);
}