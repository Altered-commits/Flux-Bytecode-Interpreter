#ifndef UNNAMED_PREPROCESSOR_HPP
#define UNNAMED_PREPROCESSOR_HPP

#include <sstream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "error_printer.hpp"

class Preprocessor
{
public:
    Preprocessor(std::string&& source)
    {
        output.str().reserve(source.size());
        preprocessInternal(std::move(source));
    }

    std::string preprocess()
    {
        return output.str();
    }

private:
    void preprocessInternal(std::string&&);
    void processLine(std::string&);

private:
    std::ostringstream output;
    std::unordered_map<std::string, std::string> macros;
};

#endif