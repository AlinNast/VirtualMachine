#pragma once

#include <cstdint>

/// <summary>
/// Interface for memory to read and write
/// </summary>
class MemorySystem
{

    public:
        virtual ~MemorySystem() = default; // destructor

        virtual uint64_t Read64(uint64_t address) = 0;
        virtual void Write64(uint64_t address, uint64_t value) = 0;

};

