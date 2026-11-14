#pragma once

#include "framework.h"
#include "memory.h"

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

	private:

		Memory& memory_;  // needs to be reference to not copy the whole mem into the cpu

		uint64_t generalRegisters_[numberOfGeneralRegisters]; // should be 4

		uint64_t instructionPointer_;  // the fift register - i think it pints to the next thing to load

};

