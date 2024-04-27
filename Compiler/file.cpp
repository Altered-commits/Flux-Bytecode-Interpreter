#include "file.hpp"

//-----------------
void FileWriter::writeToFile(const std::vector<ILCommand> &commands)
{
    for (auto &&cmd : commands)
    {
        //Write Instruction to file first
        outFile.put(static_cast<Byte>(cmd.instruction));

        //Write operand to file second, depending on what instruction, cast string operand to specific type
        switch (cmd.instruction)
        {
            //Integer operand
            case PUSH_INT:
                {
                    int val = std::stoi(cmd.operand);
                    outFile.write(reinterpret_cast<const Byte*>(&val), sizeof(int));
                }
                break;
            //Floating operand
            case PUSH_FLOAT:
                {
                    float val = std::stof(cmd.operand);
                    outFile.write(reinterpret_cast<const Byte*>(&val), sizeof(float));
                }
                break;
            
            //The actual instruction is already written, i just need to write the variable to the file
            case ACCESS_VAR:
            case ASSIGN_VAR:
                writeStringToFile(cmd.operand);
                break;
        }
    }
}

//-----------------Helper functions-----------------
void FileWriter::writeStringToFile(const std::string& str)
{
    //Write length of the string first
    std::size_t length = str.size();
    outFile.write(reinterpret_cast<const char*>(&length), sizeof(length));

    //Write the actual data now
    outFile.write(str.data(), length);
}
