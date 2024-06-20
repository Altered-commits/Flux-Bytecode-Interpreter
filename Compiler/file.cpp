#include "file.hpp"
/*
    WILL OPTIMIZE THIS IN FUTURE, RN ITS HORRENDOUS
*/

//-----------------
void FileWriter::writeToFile(const ListOfInstruction &commands)
{
    //1st pass
    try {
        for (auto &&cmd : commands)
        {
            //Write Instruction to file first
            outFile.put(static_cast<Byte>(cmd.inst));

            //Write operand to file second, depending on what instruction, cast string operand to specific type
            switch (cmd.inst)
            {
                case PUSH_INT64:
                    outFile.write(reinterpret_cast<const Byte*>(&std::get<std::int64_t>(cmd.value)), sizeof(std::int64_t));
                    break;
                case PUSH_UINT64:
                    outFile.write(reinterpret_cast<const Byte*>(&std::get<std::uint64_t>(cmd.value)), sizeof(std::uint64_t));
                    break;
                case PUSH_FLOAT:
                    outFile.write(reinterpret_cast<const Byte*>(&std::get<std::double_t>(cmd.value)), sizeof(std::double_t));
                    break;

                case ACCESS_VAR:
                case ASSIGN_VAR:
                case ASSIGN_VAR_NO_POP:
                case REASSIGN_VAR:
                case REASSIGN_VAR_NO_POP:
                //Same for these
                case DATAINST_ITER_ID:
                    writeStringToFile(std::get<std::string>(cmd.value));
                    break;
                
                case DATAINST_VAR_SCOPE_IDX:
                //Same logic for these instructions as well
                case DESTROY_MULTIPLE_SYMBOL_TABLES:
                case ITER_INIT:
                {
                    //16bit uint value
                    std::uint16_t params16Bit = cmd.scopeIndexIfNeeded;
                    outFile.write(reinterpret_cast<Byte*>(&params16Bit), sizeof(std::uint16_t));
                }
                break;
                
                case JUMP_IF_FALSE:
                case JUMP:
                //Iter has next and next / Function call and destroy / Return pretty much have same operands as jump instructions
                case ITER_HAS_NEXT:
                case ITER_NEXT:
                case FUNC_START:
                case FUNC_CALL:
                case RETURN:
                {
                    auto offset = std::get<std::size_t>(cmd.value);
                    outFile.write(reinterpret_cast<Byte*>(&offset), sizeof(std::size_t));
                }
                break;
            }
        }
    }
    catch(std::bad_variant_access bva)
    {
        std::cout << "1st PASS EXCEPTIOM: " << bva.what() << '\n';
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
