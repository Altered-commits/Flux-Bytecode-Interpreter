#include <iostream>
#include <chrono>
#include <sstream>
#include <fstream>

#include "preprocessor.hpp"
#include "parser.hpp"
#include "ilgen.hpp"

#include "file.hpp"

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    //Make sure there are exactly 2 cmd args
    if(argc != 2)
    {
        std::cout << "[USAGE]: .\\FluxCompiler filename[.txt]\n";
        std::exit(1);
    }

    //Read the source code from a file
    std::ifstream in_file{argv[1]};
    if(!in_file.is_open())
    {
        std::cout << "[CompilerError]: Failed to open file: " << argv[1] << '\n';
        std::exit(1);
    }
    std::stringstream sourceCode;
    sourceCode << in_file.rdbuf();

    //----------------COMPILATION START----------------
    auto start = std::chrono::high_resolution_clock::now();

    //Preprocessing Stage
    Preprocessor preprocess{std::move(sourceCode.str())};

    //Lexing and Parsing Stage
    Parser parser{std::move(preprocess.preprocess())};
    auto& tree = parser.parse();

    //Intermediate Language Stage
    ILGenerator ilgen{std::move(tree)};
    auto& gen_code = ilgen.generateIL();

    //Write to file
    FileWriter fw{"Gen.cflx"};
    fw.writeToFile(gen_code);
    
    //----------------COMPILATION END----------------
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Compilation Successful. Time to compile: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end - start)).count() << "ms" << '\n';

    return 0;
}