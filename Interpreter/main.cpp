#include <chrono>

#include "interpreter.hpp"

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);

    if(argc != 2)
    {
        std::cout << "[USAGE]: .\\FluxInt generated.flx\n";
        std::exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();

    ByteCodeInterpreter bct{argv[1]};
    bct.interpret();

    //End of Compilation
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time to interpret: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end - start)).count() << "ms" << '\n';
    return 0;
}