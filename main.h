#ifndef MAIN_H_
#define MAIN_H_
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#define MAIN_MEMORY_SIZE 1048576 /*2^20*/
#define MAIN_MEMORY_FETCH_DELAY 64
#define VERBOSE_MODE true
#define NUM_CORES 4

uint32_t MainMemory[MAIN_MEMORY_SIZE];

int main_memory_stalls_left;
int clk_cycles;
int cores_running;

#endif /* MAIN_H_ */
