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

void __VMInternals_Sqrt__()
{
    //Get the value which we want to sqrt of
    auto& value = globalStack.back();

    std::visit([&](auto&& arg)
    {
        if constexpr(std::is_arithmetic_v<std::decay_t<decltype(arg)>>)
            globalStack.back() = std::sqrt(arg);
        else
            printRuntimeError("BuiltinError -> Sqrt", "'Sqrt' expects argument supporting arithmetic operations");
            
    }, value);
}

void __VMInternals_Gamma__()
{
    //Get the value which we want to sqrt of
    auto& value = globalStack.back();

    std::visit([&](auto&& arg)
    {
        if constexpr(std::is_arithmetic_v<std::decay_t<decltype(arg)>>)
            globalStack.back() = std::tgamma(arg);
        else
            printRuntimeError("BuiltinError -> 'Gamma'", "'Gamma' expects argument supporting arithmetic operations");
            
    }, value);
}

void __VMInternals_GetCurrentTime__()
{
    auto now    = std::chrono::high_resolution_clock::now();
    auto nowNS  = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch  = nowNS.time_since_epoch();

    globalStack.push_back(epoch.count());
}