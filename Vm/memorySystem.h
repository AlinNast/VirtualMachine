#pragma once

#include <cstdint>

class MemorySystem
{

    public:
        virtual ~MemorySystem() = default; // destructor

        virtual uint64_t Read64(uint64_t address) = 0;
        virtual void Write64(uint64_t address, uint64_t value) = 0;

};

