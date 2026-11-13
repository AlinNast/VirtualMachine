#pragma once
#include "framework.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

// Volatile = Memory is empty at startup and forgotten at exit. Each run starts from zeros and is filled from the input file.

class Memory
{

    public:
        
        Memory(uint64_t size = Framework::RAM_SIZE) : bytes_(size, 0) {}  // the “volatile, starts empty” RAM


		uint64_t Size() const { return bytes_.size(); }  // returns the size of the memory in bytes


        /// <summary>
		/// Reads a byte from the given address in memory, (8 bits). Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <returns> a bite (uint8_t) </returns>
        uint8_t Read8(uint64_t address)
        {
            Check(address, 1);
            Count();
            return bytes_[address];
        }


        /// <summary>
		/// Writes a byte to the given address in memory, (8 bits). Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="value"></param>
        void Write8(uint64_t address, uint8_t value)
        {
            Check(address, 1);
            Count();
            bytes_[address] = value;
        }


        /// <summary>
		/// Reads a 64-bit value from the given address in memory. ( thats 8 bytes), basically like read 8 eight times. Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <returns></returns>
        uint64_t Read64(uint64_t address)
        {
			
            Check(address, 8);     // do the check but with a larger size, since we are reading 8 bytes
            Count();
            return Get64(address);
        }


        /// <summary>
		/// Writes a 64-bit value to the given address in memory. ( thats 8 bytes), basically like write 8 eight times. Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="value"></param>
        void Write64(uint64_t address, uint64_t value)
        {
            Check(address, 8);   // do the check but with a larger size, since we are reading 8 bytes
            Count();
            Put64(address, value);
        }


        /// <summary>
        ///  writes the memory just like wite but its meant to be used only at startup to fill with data
        /// </summary>
        /// <param name="address"></param>
        /// <param name="value"></param>
        void Load(uint64_t address, uint64_t value)
        {
            Check(address, 8);
            Put64(address, value);
        }


        /// <summary>
        ///  looks into memory but use only for debuginng, this will skip the clocks simulation
        /// </summary>
        /// <param name="address"></param>
        /// <returns></returns>
        uint64_t Peek64(uint64_t address) const
        {
            Check(address, 8);
            return Get64(address);
        }


        void ShowCount() {
            printf("Accesses: %llu \n",accesses);
            printf("Cycles: %llu \n", cycles);
        }



    private:
        /// <summary>
		/// the actual bytes of the memory, initialized to zero
        /// </summary>
        std::vector<uint8_t> bytes_; // this is why ram knows only bytes
        int accesses = 0;
        int cycles = 0;


        /// <summary>
        /// counts the acceses and cycles 
        /// </summary>
        void Count() 
        { 
            accesses++;
            cycles += Framework::RAM_LATENCY; 
        }


        /// <summary>
		/// Check if the address is in range of the memory, n being the size of the data to read/write. if the address is out of bounds, throws an exception.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="n"></param>
        void Check(uint64_t address, uint64_t n) const
        {
            if (n > bytes_.size() || address > bytes_.size() - n) // this checks both for "before" and "after" the momory range
                throw std::out_of_range("address outside RAM");   // needs #include <stdexcept>
        }



        /// <summary>
        /// what actually gets the data but its private
        /// </summary>
        /// <param name="address"></param>
        /// <returns></returns>
        uint64_t Get64(uint64_t address) const
        {
            uint64_t value = 0; // value to return
            for (int i = 0; i < 8; ++i)
                value |= (uint64_t)bytes_[address + i] << (8 * i);   // this << (8 * i) shifts the byte to the left by 8 bits for each iteration, so we get the next byte in the value
            return value;                                        // this |= | (bitwise OR) combines two numbers bit by bit. A result bit is 1 if either input bit is 1. When one side has zeros where the other has data, it simply pastes them together: 0xCD00 | 0xEF = 0xCDEF.
            // |= is shorthand, like += .value |= x means value = value | x.
        }

        /// <summary>
        /// what actually write the data but its private
        /// </summary>
        /// <param name="address"></param>
        /// <param name="value"></param>
        void Put64(uint64_t address, uint64_t value)
        {
            for (int i = 0; i < 8; ++i)
                bytes_[address + i] = (uint8_t)(value >> (8 * i));   // this >> (8 * i) shifts the value to the right by 8 bits for each iteration, so we get the next byte in the value
        }
};

