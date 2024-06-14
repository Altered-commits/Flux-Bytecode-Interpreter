#ifndef UNNAMED_FILE_WRITER_HPP
#define UNNAMED_FILE_WRITER_HPP

#include <fstream>

#include "ilgen.hpp"
#include "../Common/common.hpp"

using Byte = char;

class FileWriter {
    public:
        FileWriter(const char* fileName) {
            outFile.open(fileName, std::ios_base::binary);

            if (!outFile.is_open()) {
                std::cerr << "[FileWritingError] Error opening file: " << fileName << std::endl;
                std::exit(1);
            }
        }

        void writeToFile(const ListOfInstruction& commands);
    
    private:
        void writeStringToFile(const std::string& str);
    private:
        std::ofstream outFile;
};

#endif