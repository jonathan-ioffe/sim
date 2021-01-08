#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "cores.h"
#include "instruction.h"
#include "bus.h"

uint32_t MainMemory[MAIN_MEMORY_SIZE];

void init_main_memory(char* memfile){
	for (int i=0;i< MAIN_MEMORY_SIZE;i++){
		MainMemory[i]= 0;
	}
	char line[HEX_INST_LEN] = {0};
	FILE* fd = fopen(memfile,"r");
	int line_num = 0;
	while (fscanf(fd, "%s\n", line) != EOF) {
		MainMemory[line_num] = strtol(line, NULL, 16);
		line_num++;
		}
	fclose(fd);
}

void write_mem_out(uint32_t* mem, char* fname)
{
    FILE* memout_fd;

    memout_fd = fopen(fname, "w");

    int max_non_zero_mem = 0;
    for (int i = 0; i < MAIN_MEMORY_SIZE; i++)
    {
        if (mem[i] != 0)
        {
            max_non_zero_mem = i;
        }
    }

    for (int i = 0; i <= max_non_zero_mem; i++)
    {
        fprintf(memout_fd, "%08X\n", mem[i]);
    }

    fclose(memout_fd);
}


int main(int argc, char* argv[]){
    char* memout_fname;
    /*set instruction memories*/
    char* inst_mems_file_names[NUM_CORES];
    char* regout_file_names[NUM_CORES];
    char* core_trace_file_names[NUM_CORES];
    char* bus_trace_file_name;
    char* dsram_file_names[NUM_CORES];
    char* tsram_file_names[NUM_CORES];
    if (argc == 28)
    {
        for (int i = 0; i < NUM_CORES; i++) {
            inst_mems_file_names[i] = argv[i+1];
        }
        init_main_memory(argv[5]);
        memout_fname = argv[6];
        for (int i = 0; i < NUM_CORES; i++) {
            regout_file_names[i] = argv[i + 7];
        }
        for (int i = 0; i < NUM_CORES; i++) {
            core_trace_file_names[i] = argv[i + 11];
        }
        bus_trace_file_name = argv[15];
        for (int i = 0; i < NUM_CORES; i++) {
            dsram_file_names[i] = argv[i + 16];
        }
        for (int i = 0; i < NUM_CORES; i++) {
            tsram_file_names[i] = argv[i + 20];
        }
    }
    else
    {
        inst_mems_file_names[0] = "imem0.txt";
        inst_mems_file_names[1] = "imem1.txt";
        inst_mems_file_names[2] = "imem2.txt";
        inst_mems_file_names[3] = "imem3.txt";
        init_main_memory("memin.txt");
        memout_fname = "memout.txt";
        regout_file_names[0] = "regout0.txt";
        regout_file_names[1] = "regout1.txt";
        regout_file_names[2] = "regout2.txt";
        regout_file_names[3] = "regout3.txt";
        core_trace_file_names[0] = "core0trace.txt";
        core_trace_file_names[1] = "core1trace.txt";
        core_trace_file_names[2] = "core2trace.txt";
        core_trace_file_names[3] = "core3trace.txt";
        bus_trace_file_name = "bustrace.txt";
        dsram_file_names[0] = "dsram0.txt";
        dsram_file_names[1] = "dsram1.txt";
        dsram_file_names[2] = "dsram2.txt";
        dsram_file_names[3] = "dsram3.txt";
        tsram_file_names[0] = "tsram0.txt";
        tsram_file_names[1] = "tsram1.txt";
        tsram_file_names[2] = "tsram2.txt";
        tsram_file_names[3] = "tsram3.txt";
        //inst_mems_file_names[23] = "stats0.txt";
        //inst_mems_file_names[24] = "stats1.txt";
        //inst_mems_file_names[25] = "stats2.txt";
        //inst_mems_file_names[26] = "stats3.txt";
    }
    clk_cycles = 0;
    init_bus(bus_trace_file_name);
    init_cores(core_trace_file_names);
    load_inst_mems(inst_mems_file_names);

    /*set main memory*/
    run_program(MainMemory);
    write_mem_out(MainMemory, memout_fname);
    write_core_regs_files(regout_file_names);
    write_core_dsram_files(dsram_file_names);
    write_core_tsram_files(tsram_file_names);

    return 0;
}


