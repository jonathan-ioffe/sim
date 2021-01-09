#ifndef MAIN_MEMORY_H_
#define MAIN_MEMORY_H_
#include "main.h"

#define MAIN_MEMORY_SIZE 1048576 /*2^20*/
#define MAIN_MEMORY_FETCH_DELAY 64

uint32_t MainMemory[MAIN_MEMORY_SIZE];

int main_memory_stalls_left;

void init_main_memory(char** memin_fname);
void write_mem_out(char** memout_fname);
#endif /* MAIN_MEMORY_H_ */
