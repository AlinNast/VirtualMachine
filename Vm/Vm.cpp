#include <iostream>
#include "framework.h"
#include "memory.h"


int main()
{
	Memory memory;
	memory.Write64(0x100, 0x0123456789ABCDEF); // this writes the 64-bit value to memory at address 0x100
	memory.Write64(0x108, 0xACABACAB13121312); // this writes the 64 bit at 108

	// the %llx prints a hex
	printf("Read64 at 0x100: %llx\n", (unsigned long long)memory.Read64(0x100)); // 123456789abcdef
	printf("Read8  at 0x100: %x\n", memory.Read8(0x100)); // ef
	printf("Read8  at 0x107: %x\n", memory.Read8(0x107)); // 1
	printf("Read64 at 0x108: %llx\n", (unsigned long long)memory.Read64(0x108)); // 1

	memory.ShowCount();


    return 0;
}

