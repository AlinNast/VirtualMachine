#include <iostream>
#include "framework.h"
#include "memory.h"
#include "cpu.h"
#include <cstdio>


// this multiple arguments in main is like this becouse:
// argumentCount     = 2            how many words were typed
// argumentValues[0] = "Vm.exe"     the program's own name
// argumentValues[1] = "hot.txt"    the first real argument : your input file
// so basically to run it from console
int main(int argumentCount, char** argumentValues)
{
    // Step 1 --- look for the input file
    if (argumentCount < 2)
    {
        printf("Usage: Vm <input file>\n");
        return 1;
    }
    // if it works, this is the input file path
    const char* inputFilePath = argumentValues[1];


    // Step 2 --- Load the file with the framework
    Framework::Program program; 
    std::string errorMessage; // the program needs this
    bool fileWasParsed = Framework::Load(inputFilePath, program, errorMessage);
    // if loading fails becouse if the file print the error
    if (!fileWasParsed)
    {
        printf("%s\n", errorMessage.c_str());
        return 1;
    }

    printf("File:          %s\n", inputFilePath);
    printf("Data entries:  %zu\n", program.data.size());
    printf("Instructions:  %zu\n", program.code.size());

    // Step 3 --- Copy it into RAM
    const uint64_t programRegionSizeInBytes = 0x100000;   // first 1 MiB is reserved for the program
    uint64_t programSizeInBytes = program.code.size() * Cpu::bytesPerInstruction; // this reserves the size of the instruction with 8Bytes per instruction


    // just in case the program instructions is too big
    /*if (programSizeInBytes > programRegionSizeInBytes)
    {
        printf("The program is too big.\n");
        return 1;
    }*/

    Memory memory;
    for (const Framework::Program::DataEntry& dataEntry : program.data)   // data entry kind of object from farmework to iterate into the loaded data
    {
        memory.Load(dataEntry.address, dataEntry.value); // the address is in the input
    }
    for (size_t instructionIndex = 0; instructionIndex < program.code.size(); ++instructionIndex) // entry of the instructions in their reserved space
    {
        uint64_t instructionAddress = instructionIndex * Cpu::bytesPerInstruction;   
        uint64_t encodedInstruction = Framework::Encode(program.code[instructionIndex]);   // what?
        memory.Load(instructionAddress, encodedInstruction);   // writes the instruction into ram
    }

    // step 4 --- check that the ram holds the data
    size_t dataEntriesThatMatch = 0;
    for (const Framework::Program::DataEntry& dataEntry : program.data)
    {
        uint64_t valueReadBack = memory.Peek64(dataEntry.address);
        if (valueReadBack == dataEntry.value)
        {
            dataEntriesThatMatch++;
        }
    }

    // step 5 --- initialize CPU
    Cpu cpu(memory);

    cpu.Run(programSizeInBytes);



    // (the same idea for the instructions: Peek64(instructionAddress) == Framework::Encode(...)) TODO LATER

    /// all the debug prints
    

    for (uint64_t registerIndex = 0; registerIndex < Cpu::numberOfGeneralRegisters; ++registerIndex)
    {
        printf("R%llu = %llu\n", (unsigned long long)registerIndex,
            (unsigned long long)cpu.GetGeneralRegister(registerIndex));
    }
    printf("memory at 0x100000 = %llu\n", (unsigned long long)memory.Peek64(0x100000));

    Framework::Report(inputFilePath, {}, memory.stats,
        cpu.GetInstructionsExecuted(), cpu.GetExecutionCycles());

   // Framework::Instruction decodedInstruction = Framework::Decode(memory.Peek64(instructionAddress));

    return 0;
}

