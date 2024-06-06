#include <iostream>
#include <chrono>
#include <sstream>
#include <fstream>

#include "preprocessor.hpp"
#include "parser.hpp"
#include "ilgen.hpp"
#include "error_printer.hpp"
#include "../Common/common.hpp"

#include "file.hpp"

int main(int argc, char** argv)
{
    const char* const EXT = ".flux";
    
    std::ios::sync_with_stdio(false);

    //Make sure there are exactly 2 cmd args
    if(argc != 2) {
        std::cout << "[USAGE]: .\\FluxCompiler [filename].flux\n";
        std::exit(1);
    }

    //Check if the file name ends with .flux extension
    const char* filename = argv[1];
    if (!checkFileExt(EXT, filename)) {
        std::cout << "[CompilerError]: File must have a `" << EXT << "` extension: " << filename << '\n';
        std::exit(1);
    }

    //Read the source code from a file
    std::ifstream in_file{filename};
    if(!in_file.is_open()) {
        std::cout << "[CompilerError]: Failed to open file: " << filename << '\n';
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
    auto& generatedBytecode = ilgen.generateIL();

    //----------------COMPILATION END----------------
    auto end = std::chrono::high_resolution_clock::now();

    //Write to file
    FileWriter fw{"Gen.cflx"};
    fw.writeToFile(generatedBytecode);

    std::cout << "Compilation Successful. Time to compile: " <<
        (std::chrono::duration_cast<std::chrono::microseconds>(end - start)).count() << " microsec" << '\n';

    return 0;
}