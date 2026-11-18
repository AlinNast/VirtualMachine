#pragma once

#include "lineBytes.h"

#include <cstdint>

/// <summary>
/// Interface for
/// </summary>
class LineMemory
{
public:
    virtual ~LineMemory() = default;
    virtual LineBytes ReadLine(uint64_t lineStartAddress) = 0;
    virtual void WriteLine(uint64_t lineStartAddress, const LineBytes& line) = 0;
    virtual void Write64(uint64_t address, uint64_t value) = 0;
};

