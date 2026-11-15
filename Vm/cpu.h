#pragma once

#include "framework.h"
#include "memorySystem.h"
#include <cstdio>
#include <cstdint>
#include <stdexcept>

class Cpu
{
	public:
		static constexpr uint64_t numberOfGeneralRegisters = 4;  //static constexpr means “one constant that belongs to the class, known while compiling”

        static constexpr uint64_t bytesPerInstruction = 8;

        static constexpr uint64_t cyclesForMoveAddAndSubtract = 1;
        static constexpr uint64_t cyclesForMultiply = 3;


        Cpu(MemorySystem& memory) : memory_(memory)   /// constructor
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
            executionCycles_ = 0;
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
        /// For debug
        /// </summary>
        /// <returns></returns>
        uint64_t GetExecutionCycles() const 
        { 
            return executionCycles_; 
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
                Execute(instruction);                                                     // 3. EXECUTE  done

                instructionsExecuted_++;
            }
        }


	private:

        MemorySystem& memory_;  // needs to be reference to not copy the whole mem into the cpu

		uint64_t generalRegisters_[numberOfGeneralRegisters]; // should be 4

		uint64_t instructionPointer_;  // the fift register - i think it pints to the next thing to load

        uint64_t instructionsExecuted_;
        uint64_t executionCycles_;

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


        /// <summary>
        /// Picks the opCode from the instruction and pucks the right oeration
        /// </summary>
        /// <param name="instruction"></param>
        void Execute(const Framework::Instruction& instruction)   /// what instruction? i dunno
        {
            //if (instructionsExecuted_ < 3)    // only print the first three
            //{
            //    printf("opcode %llu | destination: kind %llu register %llu | source: kind %llu register %llu immediate 0x%llx\n",
            //        (unsigned long long)instruction.opcode,
            //        (unsigned long long)instruction.dst.kind, (unsigned long long)instruction.dst.reg,
            //        (unsigned long long)instruction.src.kind, (unsigned long long)instruction.src.reg,
            //        (unsigned long long)instruction.src.imm);
            //}

            switch (instruction.opcode)
            {
                case Framework::MOV:
                    ExecuteMove(instruction);
                    executionCycles_ += cyclesForMoveAddAndSubtract;
                    break;
                case Framework::ADD:
                case Framework::SUB:
                    ExecuteArithmetic(instruction);
                    executionCycles_ += cyclesForMoveAddAndSubtract;
                    break;
                case Framework::MUL:
                    ExecuteArithmetic(instruction);
                    executionCycles_ += cyclesForMultiply;
                    break;
                default:
                    throw std::runtime_error("unknown opcode");
            }
        }


        // called
        uint64_t ReadOperandValue(const Framework::Operand& operand)   
        {
            switch (operand.kind)
            {
            case Framework::REG:                    // Reg means in general register 
                return generalRegisters_[operand.reg];
            case Framework::IMM:                    // imm means in imidiate, basically in the instruction itself
                return operand.imm;
            case Framework::MEM:                    // in memory at the address in the instruction
                return memory_.Read64(operand.imm);
            case Framework::IND:                    // in memory at the address in register 
                return memory_.Read64(generalRegisters_[operand.reg]);
            }
            throw std::runtime_error("unknown operand kind");
        }


        /// <summary>
        /// Called by ExecuteMove after it gets the value to write
        /// </summary>
        /// <param name="destination"></param>
        /// <param name="value"></param>
        void WriteDestination(const Framework::Operand& destination, uint64_t value)
        {
            switch (destination.kind)
            {
            case Framework::REG:
                generalRegisters_[destination.reg] = value;
                return;
            case Framework::MEM:
                memory_.Write64(destination.imm, value);
                return;
            case Framework::IND:
                memory_.Write64(generalRegisters_[destination.reg], value);
                return;
            }
            throw std::runtime_error("this kind of operand cannot be a destination");
        }


        /// <summary>
        /// MOV - the real action
        /// </summary>
        /// <param name="instruction"></param>
        void ExecuteMove(const Framework::Instruction& instruction)
        {
            uint64_t valueToMove = ReadOperandValue(instruction.src);  // switch on the kind of src
            WriteDestination(instruction.dst, valueToMove);  // switches on the kind of dst
        }


        /// <summary>
        ///  ADD Sub Mul -- the real actions
        /// </summary>
        /// <param name="instruction"></param>
        void ExecuteArithmetic(const Framework::Instruction& instruction)
        {
            // writes to dst register only
            if (instruction.dst.kind != Framework::REG)
                throw std::runtime_error("the destination of add, sub and mul must be a register");

            // setup
            uint64_t sourceValue = ReadOperandValue(instruction.src);  
            uint64_t destinationValue = generalRegisters_[instruction.dst.reg];  // dest reg is not the same as dest kind
            uint64_t result = 0;

            // actual calculation
            switch (instruction.opcode)
            {
                case Framework::ADD: result = destinationValue + sourceValue; break;
                case Framework::SUB: result = destinationValue - sourceValue; break;
                case Framework::MUL: result = destinationValue * sourceValue; break;
            }

            // write the result in the dest reg
            generalRegisters_[instruction.dst.reg] = result;
        }


};

