#include "preprocessor.hpp"

void Preprocessor::preprocessInternal(std::string&& sourceCode)
{
    std::string line;
    std::istringstream source{std::move(sourceCode)};
    std::unordered_set<std::string> preprocessedFiles;

    while (std::getline(source, line))
    {
        if(line.empty())
            continue;

        if(line.compare(0, 7, "include") == 0)
        {
            std::string filePath = line.substr(8);
            
            //Remove whitespace
            filePath.erase(std::remove_if(filePath.begin(), filePath.end(), ::isspace), filePath.end());

            if (filePath.size() < 5 || filePath.compare(filePath.size() - 5, 5, ".flux") != 0) {
                std::cout << "PreprocessorError: Include file must have a '.flux' extension: " << filePath << '\n';
                std::exit(1);
            }

            //Replace all '.' with '/' except for the last one
            size_t lastDotPos = filePath.find_last_of('.');
            for (size_t i = 0; i < lastDotPos; ++i) {
                if (filePath[i] == '.') {
                    filePath[i] = '/';
                }
            }

            if(preprocessedFiles.find(filePath) != preprocessedFiles.end())
                continue;
            
            preprocessedFiles.insert(filePath);
            //Open file and read its content to preprocess
            std::ifstream file{filePath};
            if(!file) {
                std::cout << "PreprocessorError: " << "File not found for 'include' directive: " << filePath << '\n';
                std::exit(1);
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();

            preprocessInternal(std::move(buffer.str()));
            output << '\n';
        }
        else if(line.compare(0, 6, "define") == 0)
        {
            std::size_t keyStart = 7; //position after "define "
            std::size_t keyEnd   = line.find(' ', keyStart);
            
            if(keyEnd == std::string::npos)
            {
                std::cout << "PreprocessorError: " << "Expected key identifier after 'define'" << '\n';
                std::exit(1);
            }

            std::string key{line.data() + keyStart, keyEnd - keyStart};
            std::string value{line.data() + keyEnd + 1};

            macros[std::move(key)] = std::move(value);
        }
        else
            processLine(line);
    }
}

void Preprocessor::processLine(std::string& line)
{
    std::size_t i = 0, n = line.size();

    while(i < n)
    {
        //Identifiers
        if(std::isalnum(line[i]) || line[i] == '_')
        {
            //Start of the word
            std::size_t start = i;

            while (i < n && (std::isalnum(line[i]) || line[i] == '_'))
                ++i;
            
            std::string word{line.data() + start, i - start};

            // Check if the word is a macro
            auto it = macros.find(word);
            if (it != macros.end())
                output << it->second;
            else
                output << word;
        }
        //Any other punctuation
        else {
            output << line[i];
            ++i;
        }
    }
}
