#pragma once
#include "framework.h"
#include <vector>

class Memory
{

    public:
        
        Memory(uint64_t size = Framework::RAM_SIZE) : bytes_(size, 0) {}  // the “volatile, starts empty” RAM


		uint64_t Size() const { return bytes_.size(); }  // returns the size of the memory in bytes


    private:
        /// <summary>
		/// the actual bytes of the memory, initialized to zero
        /// </summary>
        std::vector<uint8_t> bytes_;
};

