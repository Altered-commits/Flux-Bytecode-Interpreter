#include "file.hpp"

//-----------------
void FileWriter::writeToFile(const std::vector<ILCommand> &commands)
{
    //1st pass
    try {
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
                case ASSIGN_VAR_NO_POP:
                case REASSIGN_VAR:
                case REASSIGN_VAR_NO_POP:
                    writeStringToFile(cmd.operand);
                    break;
                case DATAINST_VAR_SCOPE_IDX:
                //Same logic for this instruction as well
                case DESTROY_MULTIPLE_SYM_TABLES:
                    {
                        //16bit uint value
                        std::uint16_t scopeIndex = std::stoi(cmd.operand);
                        outFile.write(reinterpret_cast<Byte*>(&scopeIndex), sizeof(std::uint16_t));
                    }
                    break;
                
                case JUMP_IF_FALSE:
                case JUMP:
                //Iter has next and next pretty much have same operands as jump instructions
                case ITER_HAS_NEXT:
                case ITER_NEXT:
                    {
                        auto offset = std::stoull(cmd.operand);
                        //Then write value to file
                        outFile.write(reinterpret_cast<Byte*>(&offset), sizeof(std::size_t));
                    }
                    break;
                
                case DATAINST_ITER_ID:
                    writeStringToFile(cmd.operand);
                    break;
                case ITER_INIT:
                    {
                        //specifically want a 16bit uint value
                        std::uint16_t initParams = std::stoi(cmd.operand);
                        outFile.write(reinterpret_cast<Byte*>(&initParams), sizeof(std::uint16_t));
                    }
                    break;
            }
        }
    }
    catch(std::exception e)
    {
        std::cout << "1st PASS EXCEPTIOM: " << e.what() << '\n';
    }
}

//-----------------Helper functions-----------------
void FileWriter::writeStringToFile(const std::string& str)
{
    //Write length of the string first
    std::size_t length = str.size();
    outFile.write(reinterpret_cast<const Byte*>(&length), sizeof(length));

    //Write the actual data now
    outFile.write(str.data(), length);
}
