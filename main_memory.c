#include "main_memory.h"

void init_main_memory(char** memin_fname)
{
    // init the stalls left to be at max (fetching from main memory not triggered yet)
    main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;

    // first init the while memory to 0
	for (int i = 0; i < MAIN_MEMORY_SIZE; i++)
	{
		MainMemory[i] = 0;
	}
	char line[HEX_INST_LEN] = { 0 };
    // read the memory input file into the main memory
	FILE* fd = fopen(memin_fname, "r");
	int line_num = 0;
	while (fscanf(fd, "%s\n", line) != EOF)
	{
		MainMemory[line_num] = strtol(line, NULL, 16);
		line_num++;
	}
	fclose(fd);
}

void write_mem_out(char** memout_fname)
{
    // write the memory to the output file
    FILE* memout_fd;
    memout_fd = fopen(memout_fname, "w");

    // find the max index that the memory is different from 0
    int max_non_zero_mem = 0;
    for (int i = 0; i < MAIN_MEMORY_SIZE; i++)
    {
        if (MainMemory[i] != 0)
        {
            max_non_zero_mem = i;
        }
    }

    // write the relevant memory portion
    for (int i = 0; i <= max_non_zero_mem; i++)
    {
        fprintf(memout_fd, "%08X\n", MainMemory[i]);
    }

    fclose(memout_fd);
}
