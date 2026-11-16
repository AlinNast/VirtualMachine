#pragma once

#include "framework.h"

#include <cstdint>
#include <stdexcept>


/// <summary>
/// needed to cut an address ( or the result of cutting
/// </summary>
struct SplitAddress
{
    uint64_t offset;       // which byte inside the line
    uint64_t index;        // which set to look in
    uint64_t tag;          // which line it is, among all the lines that map to that set
    uint64_t lineNumber;   // address divided by the line size: identifies the line in the whole memory
};

class CacheGeometry
{
    public:
        CacheGeometry(const Framework::Config& config, uint64_t lineSizeInBytes = Framework::LINE_SIZE) : lineSizeInBytes_(lineSizeInBytes), numberOfWays_(config.ways)
        {
            // broken geometry test
            if (lineSizeInBytes_ == 0 || numberOfWays_ == 0)
                throw std::invalid_argument("line size and number of ways must not be 0");

            // size must be whole nr of sets
            if (config.size % (lineSizeInBytes_ * numberOfWays_) != 0)
                throw std::invalid_argument("cache size must be a whole number of sets");

            numberOfSets_ = config.size / (lineSizeInBytes_ * numberOfWays_);

            if (!IsPowerOfTwo(lineSizeInBytes_) || !IsPowerOfTwo(numberOfSets_))
                throw std::invalid_argument("line size and number of sets must be powers of two");

            offsetBitCount_ = BitsNeededToCount(lineSizeInBytes_);
            indexBitCount_ = BitsNeededToCount(numberOfSets_);

            uint64_t addressBitCount = BitsNeededToCount(Framework::RAM_SIZE);   // 16 MiB gives 24 bits
            uint64_t bitsUsedByOffsetAndIndex = offsetBitCount_ + indexBitCount_;
            tagBitCount_ = (bitsUsedByOffsetAndIndex < addressBitCount) ? addressBitCount - bitsUsedByOffsetAndIndex : 0;
        }

        SplitAddress Split(uint64_t address) const
        {
            SplitAddress parts;
            parts.offset = address & (lineSizeInBytes_ - 1);          // keep the lowest offsetBitCount bits
            parts.lineNumber = address >> offsetBitCount_;            // throw the offset away
            parts.index = parts.lineNumber & (numberOfSets_ - 1);     // keep the lowest indexBitCount bits of the line number
            parts.tag = parts.lineNumber >> indexBitCount_;           // throw the index away: what's left is the tag
            return parts;
        }


        uint64_t LineSizeInBytes() const { return lineSizeInBytes_; }
        uint64_t NumberOfWays() const { return numberOfWays_; }
        uint64_t NumberOfSets() const { return numberOfSets_; }
        uint64_t OffsetBitCount() const { return offsetBitCount_; }
        uint64_t IndexBitCount() const { return indexBitCount_; }
        uint64_t TagBitCount() const { return tagBitCount_; }

    private:
        // How many bits do you need to number this many different things? 64 things need 6 bits.
        static uint64_t BitsNeededToCount(uint64_t numberOfValues)
        {
            uint64_t bitCount = 0;
            while ((1ull << bitCount) < numberOfValues)
                ++bitCount;
            return bitCount;
        }

        static bool IsPowerOfTwo(uint64_t value)
        {
            return value != 0 && (1ull << BitsNeededToCount(value)) == value;
        }

        uint64_t lineSizeInBytes_;
        uint64_t numberOfWays_;
        uint64_t numberOfSets_ = 0;
        uint64_t offsetBitCount_ = 0;
        uint64_t indexBitCount_ = 0;
        uint64_t tagBitCount_ = 0;
};
