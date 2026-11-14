#pragma once

#include "framework.h"
#include "memory.h"
#include <cstdio>
#include <cstdint>
#include <stdexcept>

class Cpu
{
	public:
		static constexpr uint64_t numberOfGeneralRegisters = 4;  //static constexpr means “one constant that belongs to the class, known while compiling”

        static constexpr uint64_t bytesPerInstruction = 8;


        Cpu(Memory& memory) : memory_(memory)   /// constructor
        {
            Reset();
        }


        /// <summary>
        /// makes all the registers to be 0
        /// </summary>
        void Reset()
        {
            for (uint64_t registerIndex = 0; registerIndex < numberOfGeneralRegisters; ++registerIndex)
            {
                generalRegisters_[registerIndex] = 0;
            }
            instructionPointer_ = 0;
            instructionsExecuted_ = 0;
        }


        /// <summary>
        /// Get the 8 bytes in the register by index, 
        /// </summary>
        /// <param name="registerIndex"></param>
        /// <returns></returns>
        uint64_t GetGeneralRegister(uint64_t registerIndex) const   //  const means that this method primises to not change the state of the class
        {
            if (registerIndex >= numberOfGeneralRegisters)   // important safety check
                throw std::out_of_range("there is no such general register");
            return generalRegisters_[registerIndex];
        }
        // in reality there is no getter, its wired directly to the ALU

        /// <summary>
        ///  get the 8 byte from the instuction register
        /// </summary>
        /// <returns></returns>
        uint64_t GetInstructionPointer() const
        {
            return instructionPointer_;
        }


        /// <summary>
        /// for debug
        /// </summary>
        /// <returns></returns>
        uint64_t GetInstructionsExecuted() const 
        { 
            return instructionsExecuted_; 
        }


        /// <summary>
        /// This is the main loop of the CPU, this is the one that runs the FETCH DECODE EXECUTE, when the instruction pointer reaches program end address Run stops
        /// </summary>
        /// <param name="programEndAddress"></param>
        void Run(uint64_t programEndAddress)
        {
            while (instructionPointer_ < programEndAddress)
            {
                uint64_t instructionWord = Fetch();                                       // 1. FETCH  done
                Framework::Instruction instruction = Framework::Decode(instructionWord);  // 2. DECODE  from framework
                Execute(instruction);                                                     // 3. EXECUTE  placeholder

                instructionsExecuted_++;
            }
        }


	private:

		Memory& memory_;  // needs to be reference to not copy the whole mem into the cpu

		uint64_t generalRegisters_[numberOfGeneralRegisters]; // should be 4

		uint64_t instructionPointer_;  // the fift register - i think it pints to the next thing to load

        uint64_t instructionsExecuted_;

        /// <summary>
        /// Main part of the CPU/ Uses the IP to Read the memory -  increments IP sith 8 bytes  - then returns what it read from memory
        /// </summary>
        /// <returns></returns>
        uint64_t Fetch()
        {
            uint64_t instructionWord = memory_.Read64(instructionPointer_);
            instructionPointer_ += bytesPerInstruction;
            return instructionWord;
        }


        // placeholder for now
        void Execute(const Framework::Instruction& instruction)   /// what instruction? i dunno
        {
            if (instructionsExecuted_ < 3)    // only print the first three
            {
                printf("opcode %llu | destination: kind %llu register %llu | source: kind %llu register %llu immediate 0x%llx\n",
                    (unsigned long long)instruction.opcode,
                    (unsigned long long)instruction.dst.kind, (unsigned long long)instruction.dst.reg,
                    (unsigned long long)instruction.src.kind, (unsigned long long)instruction.src.reg,
                    (unsigned long long)instruction.src.imm);
            }
        }
};

