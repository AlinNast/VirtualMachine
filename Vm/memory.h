#pragma once
#include "framework.h"
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
        uint8_t Read8(uint64_t addr)
        {
            Check(addr, 1);
            return bytes_[addr];
        }


        /// <summary>
		/// Writes a byte to the given address in memory, (8 bits). Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="value"></param>
        void Write8(uint64_t addr, uint8_t value)
        {
            Check(addr, 1);
            bytes_[addr] = value;
        }


        /// <summary>
		/// Reads a 64-bit value from the given address in memory. ( thats 8 bytes), basically like read 8 eight times. Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <returns></returns>
        uint64_t Read64(uint64_t addr)
        {
			
            Check(addr, 8);     // do the check but with a larger size, since we are reading 8 bytes
			uint64_t value = 0; // value to return
            for (int i = 0; i < 8; ++i)
                value |= (uint64_t)bytes_[addr + i] << (8 * i);
            return value;
        }


        /// <summary>
		/// Writes a 64-bit value to the given address in memory. ( thats 8 bytes), basically like write 8 eight times. Throws an exception if the address is out of bounds.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="value"></param>
        void Write64(uint64_t addr, uint64_t value)
        {
            Check(addr, 8);   // do the check but with a larger size, since we are reading 8 bytes
            for (int i = 0; i < 8; ++i)
                bytes_[addr + i] = (uint8_t)(value >> (8 * i));
        }

    private:
        /// <summary>
		/// the actual bytes of the memory, initialized to zero
        /// </summary>
        std::vector<uint8_t> bytes_;


        /// <summary>
		/// Check if the address is in range of the memory, n being the size of the data to read/write. if the address is out of bounds, throws an exception.
        /// </summary>
        /// <param name="addr"></param>
        /// <param name="n"></param>
        void Check(uint64_t addr, uint64_t n) const
        {
            if (n > bytes_.size() || addr > bytes_.size() - n) // this checks both for "before" and "after" the momory range
                throw std::out_of_range("address outside RAM");   // needs #include <stdexcept>
        }
};

