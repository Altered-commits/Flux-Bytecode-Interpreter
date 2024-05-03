#include <iostream>
#include <chrono>
#include <sstream>
#include <fstream>

#include "parser.hpp"
#include "ilgen.hpp"

#include "file.hpp"

int main(int argc, char** argv)
{
    //Make sure there are exactly 2 cmd args
    if(argc != 2)
    {
        std::cout << "[USAGE]: .\\FluxCompiler filename[.txt]\n";
        std::exit(1);
    }

    //Start of Compilation
    std::ios::sync_with_stdio(false);
    auto start = std::chrono::high_resolution_clock::now();

    //Read the language from a file
    std::ifstream in_file{argv[1]};
    if(!in_file.is_open())
    {
        std::cout << "[CompilerError]: Failed to open file: " << argv[1] << '\n';
        std::exit(1);
    }
    std::stringstream language;
    language << in_file.rdbuf();

    //Parsing Stage
    Parser parser{language.str()};
    auto& tree = parser.parse();
    
    //Intermediate Language Stage
    ILGenerator ilgen{std::move(tree)};
    auto& gen_code = ilgen.generateIL();

    //Write to file
    FileWriter fw{"generated.flx"};
    fw.writeToFile(gen_code);
    
    //End of Compilation
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Compilation Successful. Time to compile: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end - start)).count() << "ms" << '\n';

    return 0;
}