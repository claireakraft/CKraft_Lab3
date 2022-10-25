#pragma once
/*
* Claire Kraft
* Aug 30 2022
* what is this code
*/

#ifndef CAKE
#define CAKE

#include <stdint.h>
#include <stdio.h>

void setbit(uint32_t* addr, uint8_t whichbit);
void clearbit(uint32_t* addr, uint8_t whichbit);
void setbits(uint32_t* addr, uint32_t bitmask);
void clearbits(uint32_t* addr, uint32_t bitmask);
void display_binary(uint32_t num);

#endif
