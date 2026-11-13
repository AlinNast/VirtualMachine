// framework.h - the parts of assignment 1 that are not the point of the assignment.
//
// The framework reads an input file and hands you what it contains. You implement
// the memory, the cache levels, and the CPU.
//
//   Framework::Load(path, program)        parse the file
//   Framework::Encode(instruction)        instruction -> one 64-bit word, for storing in memory
//   Framework::Decode(word)               word -> instruction, for fetching from memory
//   Framework::Stats                      the counters you are expected to report
//   Framework::Report(...)                print them
//
// Usage:
//
//   Framework::Program program;
//   std::string error;
//   if (!Framework::Load(argv[1], program, error)) { printf("%s\n", error.c_str()); return 1; }
//
//   for (auto& entry : program.data) memory.Load(entry.address, entry.value);
//   for (size_t i = 0; i < program.code.size(); ++i)
//       memory.Load(8 * i, Framework::Encode(program.code[i]));
//
//   ... run the CPU through your caches, then:
//   Framework::Report(argv[1], levels, ramStats, instructions, execCycles);

#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Framework
{

    static const size_t LINE_SIZE = 64;           // cache line size, in bytes
	static const uint64_t RAM_SIZE = 1u << 24;   // 16 MiB of main memory // memory uses this when it starts
    static const uint64_t RAM_LATENCY = 250;    // cycles for one main memory access /// = how many cycles it takes to access main memory? 250

    // ways = 1 is direct mapped, sets = 1 is fully associative.
    struct Config
    {
        uint64_t size;
        uint64_t ways;
        uint64_t latency;
    };

    static const Config CACHE_CONFIGS[3] = {
        { // L1
            .size = 32u << 10, // 32 KiB
            .ways = 8,
            .latency = 4
        },
        { // L2
            .size = 256u << 10, // 256 KiB
            .ways = 8,
            .latency = 12
        },
        { // L3
            .size = 8u << 20, // 8 MiB
            .ways = 16,
            .latency = 40
        }
    };

    enum Operation
    {
        MOV = 0,
        ADD,
        SUB,
        MUL
    };

    enum Kind
    {
        REG,  // a value in a register
        IMM,  // a value written into the instruction
        MEM,  // an address written into the instruction
        IND   // an address held in a register
    };

    struct Operand
    {
        uint64_t kind;
        uint64_t reg;
        uint64_t imm;
    };

    // An instruction is one 64-bit word: four bits per field, then the immediate.
    struct Instruction
    {
        uint64_t opcode;
        Operand dst;
        Operand src;
    };

    struct Program
    {
        struct DataEntry
        {
            uint64_t address;
            uint64_t value;
        };

        std::vector<DataEntry> data;
        std::vector<Instruction> code; // in execution order
    };

    struct Stats
    {
        std::string name;
        uint64_t accesses = 0;
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        uint64_t cycles = 0;
    };

    inline uint64_t Encode(const Instruction& instr)
    {
        return instr.opcode |
            instr.dst.reg << 4 |
            instr.src.reg << 8 |
            instr.dst.kind << 12 |
            instr.src.kind << 16 |
            (instr.dst.imm | instr.src.imm) << 20;
    }

    inline Instruction Decode(uint64_t word)
    {
        Operand dst{
            .kind = (word >> 12) & 0xF,
            .reg = (word >> 4) & 0xF,
            .imm = word >> 20
        };
        Operand src{
            .kind = (word >> 16) & 0xF,
            .reg = (word >> 8) & 0xF,
            .imm = word >> 20
        };
        return {
            .opcode = word & 0xF,
            .dst = dst,
            .src = src
        };
    }

    inline bool InRange(uint64_t addr)
    {
        return addr < RAM_SIZE;
    }

    inline uint64_t Number(const std::string& s)
    {
        return strtoull(s.c_str(), nullptr, 0);
    }

    inline Operand ParseOperand(const std::string& s)
    {
        if (s[0] == '[')
        {
            std::string inner = s.substr(1, s.size() - 1);
            if (inner[0] == 'r')
            {
                return {
                    .kind = IND,
                    .reg = (uint64_t)(inner[1] - '0'),
                    .imm = 0
                };
            }

            return {
                .kind = MEM,
                .reg = 0,
                .imm = Number(inner)
            };
        }

        if (s[0] == 'r')
        {
            return { REG, (uint64_t)(s[1] - '0'), 0 };
        }

        return {
            .kind = IMM,
            .reg = 0,
            .imm = Number(s)
        };
    }

    inline bool Load(const char* path, Program& program, std::string& error)
    {
        static const char* names[4] = { "mov", "add", "sub", "mul" };

        std::ifstream file(path);
        if (!file)
        {
            error = std::string("could not open ") + path;
            return false;
        }

        std::string line;
        bool inCode = false;

        while (getline(file, line))
        {
            line = line.substr(0, line.find(';'));
            for (char& c : line)
                if (c == ',')
                    c = ' ';

            std::istringstream in(line);
            std::string first;
            std::string second;
            std::string third;
            if (!(in >> first))
                continue;

            if (first == ".data")
            {
                inCode = false;
                continue;
            }
            if (first == ".code")
            {
                inCode = true;
                continue;
            }

            if (!inCode)
            {
                if (!(in >> second))
                {
                    error = "data line without a value: " + line;
                    return false;
                }
                uint64_t addr = Number(first);
                if (!InRange(addr))
                {
                    error = "data address outside memory: " + line;
                    return false;
                }
                program.data.push_back({
                    .address = addr,
                    .value = Number(second)
                    });
                continue;
            }

            int op = 0;
            while (op < 4 && std::string(names[op]) != first)
                ++op;
            if (op == 4)
            {
                error = "unknown instruction: " + first;
                return false;
            }
            if (!(in >> second >> third))
            {
                error = "instruction needs two operands: " + line;
                return false;
            }

            Operand d = ParseOperand(second);
            Operand s = ParseOperand(third);
            int needsImm = (d.kind == IMM || d.kind == MEM) + (s.kind == IMM || s.kind == MEM);
            if (needsImm > 1)
            {
                error = "only one operand can use the imm field: " + line;
                return false;
            }
            if ((d.kind == REG || d.kind == IND) && d.reg > 3)
            {
                error = "unknown register: " + second;
                return false;
            }
            if ((s.kind == REG || s.kind == IND) && s.reg > 3)
            {
                error = "unknown register: " + third;
                return false;
            }

            program.code.push_back({
                .opcode = (uint64_t)op,
                .dst = d,
                .src = s
                });
        }

        return true;
    }

    inline void Report(const char* path,
        const std::vector<Stats>& levels,
        const Stats& ram,
        uint64_t instructions,
        uint64_t execCycles)
    {
        uint64_t memoryCycles = ram.cycles;
        for (const Stats& level : levels)
            memoryCycles += level.cycles;

        uint64_t total = memoryCycles + execCycles;

        printf("%s\n\n", path);
        printf("instructions       %" PRIu64 "\n", instructions);
        printf("execution cycles   %" PRIu64 "\n", execCycles);
        printf("memory cycles      %" PRIu64 "\n", memoryCycles);
        printf("total cycles       %" PRIu64 "\n", total);
        double ipc = 0.0;
        double waiting = 0.0;
        if (total > 0)
        {
            ipc = (double)instructions / total;
            waiting = 100.0 * memoryCycles / total;
        }

        printf("ipc                %.2f\n", ipc);
        printf("waiting for memory %.1f%%\n\n", waiting);

        printf("level  accesses  hits  misses  evictions  cycles\n");
        for (const Stats& level : levels)
            printf("%-6s %8" PRIu64 " %5" PRIu64 " %7" PRIu64 " %9" PRIu64 " %7" PRIu64 "\n",
                level.name.c_str(), level.accesses, level.hits, level.misses,
                level.evictions, level.cycles);
        printf("%-6s %8" PRIu64 " %5s %7" PRIu64 " %9s %7" PRIu64 "\n", ram.name.c_str(),
            ram.accesses, "-", ram.misses, "-", ram.cycles);
    }

} // namespace Framework


