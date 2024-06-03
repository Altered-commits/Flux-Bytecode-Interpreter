#include <chrono>

#include "interpreter.hpp"
#include "../Common/common.hpp"

int main(int argc, char** argv)
{
    const char* const EXT = ".cflx";

    std::ios::sync_with_stdio(false);

    if(argc != 2) {
        std::cout << "[USAGE]: .\\FluxInt [filename].cflx\n";
        std::exit(1);
    }

    const char* filename = argv[1];
    if(!checkFileExt(EXT, filename)) {
        std::cout << "[InterpreterError]: File must have a `" << EXT << "` extension: " << filename << '\n';
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