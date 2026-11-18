#pragma once

#include "framework.h"

#include <array>
#include <cstdint>

// The 64 bytes of one cache line, as one value that can be copied around.
using LineBytes = std::array<uint8_t, Framework::LINE_SIZE>;   // fixed sized ROW of 64 bytes


