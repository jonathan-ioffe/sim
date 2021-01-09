#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "cores.h"
#include "instruction.h"
#include "bus.h"
#include "main_memory.h"

char* memin_fname;
char* memout_fname;
char* inst_mems_file_names[NUM_CORES];
char* regout_file_names[NUM_CORES];
char* core_trace_file_names[NUM_CORES];
char* bus_trace_file_name;
char* dsram_file_names[NUM_CORES];
char* tsram_file_names[NUM_CORES];
char* stats_file_names[NUM_CORES];

void set_file_names(bool from_argv, char* argv[])
{
    if (from_argv)
    {
        // get filenames from argv
        for (int i = 0; i < NUM_CORES; i++) {
            inst_mems_file_names[i] = argv[i + 1];
        }
        memin_fname = argv[5];
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
        for (int i = 0; i < NUM_CORES; i++) {
            stats_file_names[i] = argv[i + 24];
        }
    }
    else
    {
        // set the default filenames
        inst_mems_file_names[0] = "imem0.txt";
        inst_mems_file_names[1] = "imem1.txt";
        inst_mems_file_names[2] = "imem2.txt";
        inst_mems_file_names[3] = "imem3.txt";
        memin_fname = "memin.txt";
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
        stats_file_names[0] = "stats0.txt";
        stats_file_names[1] = "stats1.txt";
        stats_file_names[2] = "stats2.txt";
        stats_file_names[3] = "stats3.txt";
    }
}
void init_program()
{
    init_main_memory(memin_fname);
    init_bus(bus_trace_file_name);
    init_cores(inst_mems_file_names, core_trace_file_names, regout_file_names, dsram_file_names, tsram_file_names, stats_file_names);
}
void run_program()
{
    // init vars
    clk_cycles = 0;
    cores_running = NUM_CORES;

    if (VERBOSE_MODE) printf("Start running program\n");

    while (cores_running > 0)
    {
        if (VERBOSE_MODE) printf("current cycle %d\n", clk_cycles);

        // run one cycle of bus and cores
        run_bus_cycle();
        run_cores_cycle();

        // prepares the bus and cores for the next cycle
        cores_next_cycle();
        bus_next_cycle();

        clk_cycles++;
    }
}
void finish_program()
{
    // write output files
    write_mem_out(memout_fname);
    write_cores_output_files();
}

int main(int argc, char* argv[])
{
    // if provided 27 arguments, use them as input/output file names
    set_file_names(argc == 28, argv);
    
    init_program();
    run_program();
    finish_program();
    
    return 0;
}


