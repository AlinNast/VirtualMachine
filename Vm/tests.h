#pragma once

#include "cacheGeometry.h"
#include "cacheLevel.h"
#include "framework.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace Tests
{
    inline int numberOfChecks = 0;
    inline int numberOfFailedChecks = 0;

    inline void CheckEqual(const char* description, uint64_t actual, uint64_t expected)
    {
        numberOfChecks++;
        if (actual != expected)
        {
            numberOfFailedChecks++;
            printf("FAILED: %s: expected %llu (0x%llx), got %llu (0x%llx)\n", description,
                (unsigned long long)expected, (unsigned long long)expected,
                (unsigned long long)actual, (unsigned long long)actual);
        }
    }

    inline bool GeometryIsRejected(uint64_t sizeInBytes, uint64_t numberOfWays)
    {
        try
        {
            Framework::Config badConfig{ sizeInBytes, numberOfWays, 4 };
            CacheGeometry geometry(badConfig);
        }
        catch (const std::invalid_argument&)
        {
            return true;
        }
        return false;
    }

    inline void TestGeometryOfTheDefaultLevels()
    {
        CacheGeometry level1(Framework::CACHE_CONFIGS[0]);
        CheckEqual("L1 sets", level1.NumberOfSets(), 64);
        CheckEqual("L1 offset bits", level1.OffsetBitCount(), 6);
        CheckEqual("L1 index bits", level1.IndexBitCount(), 6);
        CheckEqual("L1 tag bits", level1.TagBitCount(), 12);

        CacheGeometry level2(Framework::CACHE_CONFIGS[1]);
        CheckEqual("L2 sets", level2.NumberOfSets(), 512);
        CheckEqual("L2 index bits", level2.IndexBitCount(), 9);
        CheckEqual("L2 tag bits", level2.TagBitCount(), 9);

        CacheGeometry level3(Framework::CACHE_CONFIGS[2]);
        CheckEqual("L3 sets", level3.NumberOfSets(), 8192);
        CheckEqual("L3 index bits", level3.IndexBitCount(), 13);
        CheckEqual("L3 tag bits", level3.TagBitCount(), 5);
    }

    inline void TestGeometryOfOtherMappings()
    {
        Framework::Config directMappedConfig{ 32 * 1024, 1, 4 };
        CacheGeometry directMapped(directMappedConfig);
        CheckEqual("direct mapped sets", directMapped.NumberOfSets(), 512);
        CheckEqual("direct mapped index bits", directMapped.IndexBitCount(), 9);
        CheckEqual("direct mapped tag bits", directMapped.TagBitCount(), 9);

        Framework::Config fullyAssociativeConfig{ 32 * 1024, 512, 4 };
        CacheGeometry fullyAssociative(fullyAssociativeConfig);
        CheckEqual("fully associative sets", fullyAssociative.NumberOfSets(), 1);
        CheckEqual("fully associative index bits", fullyAssociative.IndexBitCount(), 0);
        CheckEqual("fully associative tag bits", fullyAssociative.TagBitCount(), 18);
    }

    inline void TestSplitOnTheEightWayL1()
    {
        CacheGeometry level1(Framework::CACHE_CONFIGS[0]);

        SplitAddress parts = level1.Split(0x100008);
        CheckEqual("0x100008 offset", parts.offset, 8);
        CheckEqual("0x100008 line number", parts.lineNumber, 0x4000);
        CheckEqual("0x100008 index", parts.index, 0);
        CheckEqual("0x100008 tag", parts.tag, 0x100);

        parts = level1.Split(0x101000);                       // the conflict.txt neighbour
        CheckEqual("0x101000 index", parts.index, 0);
        CheckEqual("0x101000 tag", parts.tag, 0x101);

        parts = level1.Split(0x10003F);                       // last byte of the line that starts at 0x100000
        CheckEqual("0x10003F offset", parts.offset, 63);
        CheckEqual("0x10003F index", parts.index, 0);

        parts = level1.Split(0x100040);                       // first byte of the NEXT line
        CheckEqual("0x100040 offset", parts.offset, 0);
        CheckEqual("0x100040 index", parts.index, 1);
        CheckEqual("0x100040 tag", parts.tag, 0x100);

        parts = level1.Split(0);
        CheckEqual("address 0 offset", parts.offset, 0);
        CheckEqual("address 0 index", parts.index, 0);
        CheckEqual("address 0 tag", parts.tag, 0);

        parts = level1.Split(0xFFFFFF);                       // the very last address
        CheckEqual("0xFFFFFF offset", parts.offset, 63);
        CheckEqual("0xFFFFFF index", parts.index, 63);
        CheckEqual("0xFFFFFF tag", parts.tag, 0xFFF);
    }

    inline void TestTheConflictOnADirectMappedL1()
    {
        Framework::Config directMappedConfig{ 32 * 1024, 1, 4 };
        CacheGeometry directMapped(directMappedConfig);

        SplitAddress firstParts = directMapped.Split(0x100000);
        SplitAddress secondParts = directMapped.Split(0x108000);
        CheckEqual("A index", firstParts.index, 0);
        CheckEqual("A tag", firstParts.tag, 32);
        CheckEqual("B index", secondParts.index, 0);
        CheckEqual("B tag", secondParts.tag, 33);
        CheckEqual("A and B fight for the same slot", firstParts.index == secondParts.index && firstParts.tag != secondParts.tag, 1);
    }

    inline void TestSplittingThenRebuilding()
    {
        CacheGeometry level1(Framework::CACHE_CONFIGS[0]);

        SplitAddress parts = level1.Split(0x100008);
        CheckEqual("rebuild 0x100008", level1.AddressOfLine(parts.tag, parts.index), 0x100000);

        parts = level1.Split(0xFFFFFF);
        CheckEqual("rebuild 0xFFFFFF", level1.AddressOfLine(parts.tag, parts.index), 0xFFFFC0);

        for (uint64_t address = 0; address < 0x2000000; address += 4099)   // many different addresses
        {
            if (address >= Framework::RAM_SIZE) break;
            SplitAddress loopParts = level1.Split(address);
            uint64_t lineStart = level1.AddressOfLine(loopParts.tag, loopParts.index);
            if (lineStart + loopParts.offset != address)
                CheckEqual("rebuild loop", lineStart + loopParts.offset, address);
        }
        CheckEqual("rebuild loop finished", 1, 1);
    }

    inline void TestBadGeometriesAreRejected()
    {
        CheckEqual("3 ways cannot divide 32 KiB into whole sets", GeometryIsRejected(32 * 1024, 3), 1);
        CheckEqual("0 ways", GeometryIsRejected(32 * 1024, 0), 1);
        CheckEqual("size smaller than one line", GeometryIsRejected(32, 1), 1);
        CheckEqual("a good geometry is accepted", GeometryIsRejected(32 * 1024, 8), 0);
    }

    inline void TestRamLines()
    {
        Memory ram;
        ram.Load(0x100000, 0x0807060504030201ull);   // bytes 01 02 ... 08 at 0x100000..0x100007
        ram.Load(0x100008, 0xFFull);                  // byte FF at 0x100008, zeros after it

        LineBytes line = ram.ReadLine(0x100000);
        CheckEqual("line byte 0 is the lowest byte", line[0], 0x01);
        CheckEqual("line byte 7 is the highest byte", line[7], 0x08);
        CheckEqual("line byte 8", line[8], 0xFF);
        CheckEqual("line byte 9", line[9], 0x00);
        CheckEqual("line byte 63", line[63], 0x00);
        CheckEqual("reading a line is ONE access", ram.stats.accesses, 1);
        CheckEqual("reading a line costs 250 cycles", ram.stats.cycles, 250);

        LineBytes newLine;
        for (uint64_t byteInLine = 0; byteInLine < newLine.size(); ++byteInLine)
            newLine[byteInLine] = (uint8_t)(byteInLine + 100);
        ram.WriteLine(0x100040, newLine);
        CheckEqual("writing a line is ONE more access", ram.stats.accesses, 2);
        CheckEqual("two line accesses cost 500 cycles", ram.stats.cycles, 500);
        CheckEqual("Peek64 sees the written line (bytes 100..107)", ram.Peek64(0x100040), 0x6B6A696867666564ull);
        CheckEqual("last byte of the written line", ram.ReadLine(0x100040)[63], 163);
        CheckEqual("the line before is untouched", ram.Peek64(0x100038), 0);
        CheckEqual("the line after is untouched", ram.Peek64(0x100080), 0);

        ram.WriteLine(0xFFFFC0, newLine);   // the very last line of RAM is allowed
        CheckEqual("last line of RAM", ram.ReadLine(0xFFFFC0)[0], 100);

        int rejected = 0;
        try { ram.ReadLine(0x100008); }
        catch (const std::invalid_argument&) { rejected++; }
        CheckEqual("an address in the middle of a line is rejected", rejected, 1);
        try { ram.WriteLine(0x100001, newLine); }
        catch (const std::invalid_argument&) { rejected++; }
        CheckEqual("a misplaced write is rejected", rejected, 2);
        try { ram.ReadLine(0x1000000); }
        catch (const std::out_of_range&) { rejected++; }
        CheckEqual("a line outside RAM is rejected", rejected, 3);
        CheckEqual("rejected calls cost nothing: still 5 accesses", ram.stats.accesses, 5);
    }

    inline void TestCacheLevelSkeleton()
    {
        Memory ramForSkeleton;
        CacheLevel level1("L1", Framework::CACHE_CONFIGS[0], ramForSkeleton);
        CheckEqual("L1 name", level1.stats.name == "L1", 1);
        CheckEqual("L1 latency", level1.LatencyInCycles(), 4);
        CheckEqual("L1 sets", level1.Geometry().NumberOfSets(), 64);
        CheckEqual("L1 ways", level1.Geometry().NumberOfWays(), 8);
        CheckEqual("L1 starts with no valid line", level1.CountValidLines(), 0);
        CheckEqual("first slot is empty", level1.SlotAt(0, 0).isValid, 0);
        CheckEqual("last slot exists and is empty", level1.SlotAt(63, 7).isValid, 0);
        CheckEqual("an empty slot has zero bytes", level1.SlotAt(63, 7).bytes[63], 0);
        CheckEqual("counters start at zero", level1.stats.accesses + level1.stats.hits + level1.stats.misses
            + level1.stats.evictions + level1.stats.cycles, 0);

        CacheLevel level3("L3", Framework::CACHE_CONFIGS[2], ramForSkeleton);
        CheckEqual("L3 sets", level3.Geometry().NumberOfSets(), 8192);
        CheckEqual("L3 latency", level3.LatencyInCycles(), 40);
        CheckEqual("L3 last slot exists", level3.SlotAt(8191, 15).isValid, 0);

        int rejected = 0;
        try { level1.SlotAt(64, 0); }
        catch (const std::out_of_range&) { rejected++; }
        try { level1.SlotAt(0, 8); }
        catch (const std::out_of_range&) { rejected++; }
        CheckEqual("slots outside the grid are rejected", rejected, 2);

        Framework::Config directMapped = Framework::CACHE_CONFIGS[0];
        directMapped.ways = 1;
        CacheLevel directMappedLevel("L1 direct", directMapped, ramForSkeleton);
        CheckEqual("direct-mapped: 512 sets", directMappedLevel.Geometry().NumberOfSets(), 512);
        CheckEqual("direct-mapped: slot (511,0) exists", directMappedLevel.SlotAt(511, 0).isValid, 0);
    }

    inline void TestReadPathOnOneLevel()
    {
        Memory ram;
        ram.Load(0x100000, 0x1111111111111111ull);
        ram.Load(0x100008, 0x2222222222222222ull);
        ram.Load(0x100040, 0x3333333333333333ull);
        CacheLevel level1("L1", Framework::CACHE_CONFIGS[0], ram);

        CheckEqual("first read: right value", level1.Read64(0x100000), 0x1111111111111111ull);
        CheckEqual("first read: 1 access", level1.stats.accesses, 1);
        CheckEqual("first read: a miss", level1.stats.misses, 1);
        CheckEqual("first read: no hit", level1.stats.hits, 0);
        CheckEqual("first read: a miss costs nothing at L1", level1.stats.cycles, 0);
        CheckEqual("first read: RAM was asked once", ram.stats.accesses, 1);
        CheckEqual("first read: RAM cycles", ram.stats.cycles, 250);
        CheckEqual("the line is now stored", level1.CountValidLines(), 1);
        CheckEqual("stored in set 0", level1.SlotAt(0, 0).isValid, 1);
        CheckEqual("with tag 0x100", level1.SlotAt(0, 0).tag, 0x100);

        CheckEqual("same address again: right value", level1.Read64(0x100000), 0x1111111111111111ull);
        CheckEqual("same address again: a hit", level1.stats.hits, 1);
        CheckEqual("a hit costs the L1 latency", level1.stats.cycles, 4);
        CheckEqual("a hit does not touch RAM", ram.stats.accesses, 1);

        CheckEqual("neighbour in the same line: right value", level1.Read64(0x100008), 0x2222222222222222ull);
        CheckEqual("neighbour in the same line: a hit", level1.stats.hits, 2);
        CheckEqual("two hits cost 8 cycles", level1.stats.cycles, 8);
        CheckEqual("RAM still asked once", ram.stats.accesses, 1);

        CheckEqual("next line: right value", level1.Read64(0x100040), 0x3333333333333333ull);
        CheckEqual("next line: a miss", level1.stats.misses, 2);
        CheckEqual("next line: RAM asked twice", ram.stats.accesses, 2);
        CheckEqual("two lines stored", level1.CountValidLines(), 2);
        CheckEqual("the next line is in set 1", level1.SlotAt(1, 0).isValid, 1);

        int rejected = 0;
        try { level1.ReadLine(0x100008); }
        catch (const std::invalid_argument&) { rejected++; }
        try { level1.Read64(0x10003C); }
        catch (const std::invalid_argument&) { rejected++; }
        CheckEqual("unaligned line and line-crossing read are rejected", rejected, 2);
    }

    inline void TestEmptySlotsNeverHit()
    {
        Memory ram;
        ram.Load(0, 0x77);   // address 0 has tag 0, the same as an empty slot
        CacheLevel level1("L1", Framework::CACHE_CONFIGS[0], ram);
        CheckEqual("address 0 on an empty cache: right value", level1.Read64(0), 0x77);
        CheckEqual("address 0 on an empty cache: a miss", level1.stats.misses, 1);
        CheckEqual("address 0 on an empty cache: no hit", level1.stats.hits, 0);
    }

    inline void TestConflictsOnOneLevel()
    {
        Memory ram;
        Framework::Config directMapped = Framework::CACHE_CONFIGS[0];
        directMapped.ways = 1;
        CacheLevel level1("L1 direct", directMapped, ram);

        level1.Read64(0x100000);   // A: miss, empty slot
        level1.Read64(0x108000);   // B: same set, other tag: miss, throws A out
        CheckEqual("B evicted A", level1.stats.evictions, 1);
        level1.Read64(0x100000);   // A again: miss, throws B out
        CheckEqual("A -> B -> A is 3 misses", level1.stats.misses, 3);
        CheckEqual("A -> B -> A has no hit", level1.stats.hits, 0);
        CheckEqual("A -> B -> A: 2 evictions", level1.stats.evictions, 2);
        CheckEqual("A -> B -> A: RAM asked 3 times", ram.stats.accesses, 3);

        Memory ramForEightWays;
        CacheLevel eightWays("L1", Framework::CACHE_CONFIGS[0], ramForEightWays);
        for (uint64_t lineNumber = 0; lineNumber < 8; ++lineNumber)
            eightWays.Read64(0x100000 + lineNumber * 0x1000);   // 8 lines for set 0: they all fit
        CheckEqual("8 lines in an 8-way set: 8 misses", eightWays.stats.misses, 8);
        CheckEqual("8 lines in an 8-way set: no eviction", eightWays.stats.evictions, 0);
        eightWays.Read64(0x100000 + 8 * 0x1000);               // the 9th line: one must go
        CheckEqual("the 9th line evicts one", eightWays.stats.evictions, 1);
        eightWays.Read64(0x100000 + 1 * 0x1000);               // an old line that must still be there
        CheckEqual("line 1 survived: a hit", eightWays.stats.hits, 1);
        eightWays.Read64(0x100000);                            // the first line was the one thrown out
        CheckEqual("line 0 was thrown out: a miss", eightWays.stats.misses, 10);
    }

    inline void TestTwoLevelsStacked()
    {
        Memory ram;
        ram.Load(0x100000, 0xAAull);
        ram.Load(0x108000, 0xBBull);
        CacheLevel level2("L2", Framework::CACHE_CONFIGS[1], ram);
        Framework::Config directMapped = Framework::CACHE_CONFIGS[0];
        directMapped.ways = 1;
        CacheLevel level1("L1", directMapped, level2);

        CheckEqual("A through two levels", level1.Read64(0x100000), 0xAA);
        CheckEqual("L1 missed", level1.stats.misses, 1);
        CheckEqual("L2 missed too", level2.stats.misses, 1);
        CheckEqual("RAM answered", ram.stats.accesses, 1);
        CheckEqual("both levels keep a copy", level1.CountValidLines() + level2.CountValidLines(), 2);

        CheckEqual("B through two levels", level1.Read64(0x108000), 0xBB);   // throws A out of the direct-mapped L1
        CheckEqual("RAM answered twice", ram.stats.accesses, 2);
        CheckEqual("A again, now answered by L2", level1.Read64(0x100000), 0xAA);
        CheckEqual("L1 missed 3 times", level1.stats.misses, 3);
        CheckEqual("L2 hit once", level2.stats.hits, 1);
        CheckEqual("an L2 hit costs 12", level2.stats.cycles, 12);
        CheckEqual("RAM was not asked a third time", ram.stats.accesses, 2);
        CheckEqual("RAM cycles", ram.stats.cycles, 500);
    }

    inline void TestWritePath()
    {
        Memory ram;
        ram.Load(0x100000, 0x11);
        ram.Load(0x100008, 0x22);
        CacheLevel level1("L1", Framework::CACHE_CONFIGS[0], ram);

        level1.Write64(0x200000, 0x55);   // a write to an address that is not in the cache
        CheckEqual("write miss: RAM has the value", ram.Peek64(0x200000), 0x55);
        CheckEqual("write miss: counted as an access", level1.stats.accesses, 1);
        CheckEqual("write miss: counted as a miss", level1.stats.misses, 1);
        CheckEqual("write miss: no cycles at L1", level1.stats.cycles, 0);
        CheckEqual("write miss: the line is not brought in", level1.CountValidLines(), 0);
        CheckEqual("write miss: RAM accesses", ram.stats.accesses, 1);
        CheckEqual("write miss: RAM cycles", ram.stats.cycles, 250);
        CheckEqual("reading it back: right value", level1.Read64(0x200000), 0x55);
        CheckEqual("reading it back: a miss, then stored", level1.CountValidLines(), 1);

        level1.Read64(0x100000);          // now 0x100000 is cached
        level1.Write64(0x100000, 0x99);   // write hit
        CheckEqual("write hit: RAM has the value too", ram.Peek64(0x100000), 0x99);
        CheckEqual("write hit: counted as a hit", level1.stats.hits, 1);
        CheckEqual("write hit: the cached copy is updated", level1.Read64(0x100000), 0x99);
        CheckEqual("write hit: that read was a hit", level1.stats.hits, 2);
        CheckEqual("write hit: only the read hit cost cycles", level1.stats.cycles, 4);
        CheckEqual("the neighbour in the same line is untouched", level1.Read64(0x100008), 0x22);

        Memory untouchedRam;
        CacheLevel untouchedLevel("L1", Framework::CACHE_CONFIGS[0], untouchedRam);
        int rejected = 0;
        try { untouchedLevel.Write64(0x10003C, 1); }
        catch (const std::invalid_argument&) { rejected++; }
        CheckEqual("a write crossing a line end is rejected", rejected, 1);
        CheckEqual("a rejected write changes nothing", untouchedRam.stats.accesses, 0);
    }

    inline void TestWriteThroughTwoLevels()
    {
        Memory ram;
        ram.Load(0x100000, 0xAA);
        ram.Load(0x108000, 0xBB);
        CacheLevel level2("L2", Framework::CACHE_CONFIGS[1], ram);
        Framework::Config directMapped = Framework::CACHE_CONFIGS[0];
        directMapped.ways = 1;
        CacheLevel level1("L1", directMapped, level2);

        level1.Read64(0x100000);          // A is now in L1 and in L2
        level1.Write64(0x100000, 0x99);   // the write must reach both copies and RAM
        CheckEqual("L1 hit on the write", level1.stats.hits, 1);
        CheckEqual("L2 hit on the write", level2.stats.hits, 1);
        CheckEqual("RAM has the new value", ram.Peek64(0x100000), 0x99);
        level1.Read64(0x108000);          // B throws A out of the direct-mapped L1
        CheckEqual("A is read again from L2: new value, not the old one", level1.Read64(0x100000), 0x99);
        CheckEqual("that was answered by L2", level2.stats.hits, 2);
        CheckEqual("RAM accesses: read A, write A, read B", ram.stats.accesses, 3);
    }

    inline void TestLeastRecentlyUsedReplacement()
    {
        const uint64_t distanceBetweenLinesOfOneSet = 0x1000;   // same index (0) on the 8-way L1, different tags
        const uint64_t firstAddress = 0x100000;

        Memory ram;
        CacheLevel level1("L1", Framework::CACHE_CONFIGS[0], ram);
        for (uint64_t lineNumber = 0; lineNumber < 8; ++lineNumber)
            level1.Read64(firstAddress + lineNumber * distanceBetweenLinesOfOneSet);   // fills the 8 ways: A0 is the oldest

        level1.Read64(firstAddress);                                                   // use A0 again: now A1 is the oldest
        CheckEqual("re-reading A0 is a hit", level1.stats.hits, 1);
        level1.Read64(firstAddress + 8 * distanceBetweenLinesOfOneSet);                // A8: the set is full, one line must go
        CheckEqual("one eviction", level1.stats.evictions, 1);
        level1.Read64(firstAddress);
        CheckEqual("A0 was protected by its recent use: a hit", level1.stats.hits, 2);
        level1.Read64(firstAddress + 1 * distanceBetweenLinesOfOneSet);
        CheckEqual("A1 was the least recently used: it was thrown out, a miss", level1.stats.misses, 10);
        CheckEqual("that miss evicted another line", level1.stats.evictions, 2);

        Memory secondRam;
        CacheLevel secondLevel1("L1", Framework::CACHE_CONFIGS[0], secondRam);
        for (uint64_t lineNumber = 0; lineNumber < 8; ++lineNumber)
            secondLevel1.Read64(firstAddress + lineNumber * distanceBetweenLinesOfOneSet);
        secondLevel1.Write64(firstAddress, 5);                                         // a write to A0 is a use of A0
        secondLevel1.Read64(firstAddress + 8 * distanceBetweenLinesOfOneSet);          // evicts A1, not A0
        secondLevel1.Read64(firstAddress);
        CheckEqual("a write also protects a line: read A0 is a hit", secondLevel1.stats.hits, 2);   // the write hit + this read

        Memory cyclingRam;
        CacheLevel cyclingLevel("L1", Framework::CACHE_CONFIGS[0], cyclingRam);
        for (int round = 0; round < 3; ++round)
            for (uint64_t lineNumber = 0; lineNumber < 9; ++lineNumber)
                cyclingLevel.Read64(firstAddress + lineNumber * distanceBetweenLinesOfOneSet);
        CheckEqual("9 lines cycled through 8 ways: LRU never hits (the classic worst case)", cyclingLevel.stats.hits, 0);
    }

    inline int RunAll()
    {
        TestGeometryOfTheDefaultLevels();
        TestGeometryOfOtherMappings();
        TestSplitOnTheEightWayL1();
        TestTheConflictOnADirectMappedL1();
        TestSplittingThenRebuilding();
        TestBadGeometriesAreRejected();
        TestRamLines();
        TestCacheLevelSkeleton();
        TestReadPathOnOneLevel();
        TestEmptySlotsNeverHit();
        TestConflictsOnOneLevel();
        TestTwoLevelsStacked();
        TestWritePath();
        TestWriteThroughTwoLevels();
        TestLeastRecentlyUsedReplacement();

        printf("%d checks, %d failed\n", numberOfChecks, numberOfFailedChecks);
        return numberOfFailedChecks == 0 ? 0 : 1;
    }
}
