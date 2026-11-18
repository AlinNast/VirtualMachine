#pragma once

#include "cacheGeometry.h"
#include "framework.h"
#include "lineBytes.h"
#include "lineMemory.h"
#include "memorySystem.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// One slot of a cache set: either empty, or holding a copy of one line of RAM.
struct CacheLine
{
    bool isValid = false;   // false = empty slot, the other fields mean nothing
    uint64_t tag = 0;       // which line of RAM this slot holds (among those that map to its set)
    LineBytes bytes{};      // the copy of the 64 bytes
    uint64_t lastUsedTime = 0;   // value of the level's use counter at the last read, write or fill of this line
};

class CacheLevel : public MemorySystem, public LineMemory
{
    public:
        CacheLevel(const std::string& name, const Framework::Config& config, LineMemory& below) : geometry_(config), latencyInCycles_(config.latency), sets_(geometry_.NumberOfSets(), std::vector<CacheLine>(geometry_.NumberOfWays())), below_(below)
        {
            stats.name = name;
        }

        // Read-only look at one slot, for tests and for tracing later.
        const CacheLine& SlotAt(uint64_t setIndex, uint64_t wayIndex) const
        {
            if (setIndex >= sets_.size() || wayIndex >= sets_[setIndex].size())
                throw std::out_of_range("no such slot in this cache");
            return sets_[setIndex][wayIndex];
        }

        uint64_t CountValidLines() const
        {
            uint64_t numberOfValidLines = 0;
            for (const std::vector<CacheLine>& set : sets_)
                for (const CacheLine& slot : set)
                    if (slot.isValid)
                        ++numberOfValidLines;
            return numberOfValidLines;
        }

        /// <summary>
        /// gets the address from the line start ( thats multiples of line size), selets the right set
        /// </summary>
        /// <param name="lineStartAddress"></param>
        /// <returns></returns>
        LineBytes ReadLine(uint64_t lineStartAddress) override
        {
            SplitAddress parts = geometry_.Split(lineStartAddress);

            // safety check
            if (parts.offset != 0)
                throw std::invalid_argument("a line must start at a multiple of the line size");
            ++stats.accesses;

            std::vector<CacheLine>& set = sets_[parts.index]; // select the right set

            CacheLine* foundLine = FindLineInSet(set, parts.tag);  // returns null if cant find something

            // HIT CASE
            if (foundLine != nullptr)                       
            {
                ++stats.hits;
                stats.cycles += latencyInCycles_;           // metrics
                foundLine->lastUsedTime = ++useCounter_;    // used just now

                return foundLine->bytes;
            }

            // MISS CASE
            ++stats.misses;                                 
            LineBytes lineFromBelow = below_.ReadLine(lineStartAddress);  // apply on the memory below

            // the returned line from below is written in this cache two
            CacheLine& slot = ChooseSlotToFill(set);  // what slot in the set this line whould fill 
            slot.isValid = true;
            slot.tag = parts.tag;
            slot.bytes = lineFromBelow;                 // set it up
            slot.lastUsedTime = ++useCounter_;          // filled just now

            return lineFromBelow;
        }


        // Writing a whole line is only needed by write-back (an optional step later). Not used with write-through.
        void WriteLine(uint64_t, const LineBytes&) override
        {
            throw std::logic_error("WriteLine is not needed with write-through");
        }

        /// <summary>
        /// The write in cache is upgraded to also write in all the lines in the already written cache if that address already exists, all the way to the ram
        /// </summary>
        /// <param name="address"></param>
        /// <param name="value"></param>
        void Write64(uint64_t address, uint64_t value) override
        {
            // split so it can be checked if a line containing already exists
            SplitAddress parts = geometry_.Split(address);
            // safety check
            if (parts.offset + 8 > geometry_.LineSizeInBytes())
                throw std::invalid_argument("a 64-bit write must not cross the end of a line");

            ++stats.accesses;

            // looks if a line for that address is already written in cache and writes there too before moving on
            CacheLine* foundLine = FindLineInSet(sets_[parts.index], parts.tag);
            if (foundLine != nullptr)                       // keep our copy up to date
            {
                ++stats.hits;
                for (uint64_t byteIndex = 0; byteIndex < 8; ++byteIndex)
                    foundLine->bytes[parts.offset + byteIndex] = (uint8_t)(value >> (8 * byteIndex));
                foundLine->lastUsedTime = ++useCounter_;    // a write is a use too
            }
            else
            {
                ++stats.misses;                             // it doesent matter that much to have a miss here but turns out there is a convention to count it
            }

            below_.Write64(address, value);                 // the level below (finally RAM) charges the cost
        }


        /// <summary>
        /// Reading 8 bytes in cache is not like in RAM, first its slices the address and looks for a corresponding line
        /// </summary>
        /// <param name="address"></param>
        /// <returns></returns>
        uint64_t Read64(uint64_t address) override
        {
            SplitAddress parts = geometry_.Split(address);

            // safe check for unusual sizez
            if (parts.offset + 8 > geometry_.LineSizeInBytes())
                throw std::invalid_argument("a 64-bit read must not cross the end of a line");

            LineBytes line = ReadLine(address - parts.offset); // the -offset means that the read line is called with the start of the address that will indicate the right set index

            uint64_t value = 0;
            for (uint64_t byteIndex = 0; byteIndex < 8; ++byteIndex)
                value |= (uint64_t)line[parts.offset + byteIndex] << (8 * byteIndex);
            return value;
        }

        const CacheGeometry& Geometry() const { return geometry_; }
        uint64_t LatencyInCycles() const { return latencyInCycles_; }

        Framework::Stats stats;

    private:

        // Returns the valid slot of this set that holds the line with this tag, or nullptr if there is none.
        CacheLine* FindLineInSet(std::vector<CacheLine>& set, uint64_t tag)
        {
            for (CacheLine& slot : set)
                if (slot.isValid && slot.tag == tag)
                    return &slot;
            return nullptr;
        }

        /// <summary>
        /// the right set come from splitting the address geometry 
        /// </summary>
        /// <param name="set"></param>
        /// <returns></returns>
        CacheLine& ChooseSlotToFill(std::vector<CacheLine>& set)
        {
            /// this checks for the first empty slot
            for (CacheLine& slot : set)
                if (!slot.isValid)
                    return slot;

            // if that doesent return start checking the least recent from the begining of the set
            CacheLine* leastRecentlyUsed = &set[0];
            for (CacheLine& slot : set)
                if (slot.lastUsedTime < leastRecentlyUsed->lastUsedTime)
                    leastRecentlyUsed = &slot;

            ++stats.evictions; // measure the evictions
            return *leastRecentlyUsed;  // this is LRU
        }



        CacheGeometry geometry_;
        uint64_t latencyInCycles_;
        std::vector<std::vector<CacheLine>> sets_;   // sets_[setIndex][wayIndex]
        LineMemory& below_;
        uint64_t useCounter_ = 0;   // goes up by one at every use of any line of this level
};

