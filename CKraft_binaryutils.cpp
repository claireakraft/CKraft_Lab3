/*
* Claire Kraft
* Aug 30 2022
* what is this code
*/

#include "CKraft_binaryutils.h"

// this function sets the bits of the specified address at the specified bit
void setbit(uint32_t* addr, uint8_t whichbit) {
	uint32_t num;
	num = 1 << whichbit;
	*addr = num | *addr;
}

// this function clears the bits of the specified address at the specified bit
void clearbit(uint32_t* addr, uint8_t whichbit) {
	uint32_t num;
	num = 1 << whichbit;
	num = ~(num);
	*addr = num & *addr;
}

// this function sets the bits defined the the bitmask
void setbits(uint32_t* addr, uint32_t bitmask) {
	*addr = *addr | bitmask;
}

// this function clears the bits defined the the bitmask
void clearbits(uint32_t* addr, uint32_t bitmask) {
	*addr = *addr & (~(bitmask));
}

// this function prints out the binary form of the 32 bit number entered
void display_binary(uint32_t num) {
	// for loop to cycle through each bit 
	for (int i = 31; i > -1; i--) {
		uint32_t bitmask;
		uint32_t temp;
		bitmask = 1 << i;
		temp = bitmask & num;
		// if else statements to print the 0 or 1s
		if ( temp == 0) {
			printf("0");
		}
		else {
			printf("1");
		}
	}
	printf("\n");
}

