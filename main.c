#include <stdio.h>
#include <stdlib.h>
#include "cores.h"
#include "instruction.h"
#include "bus.h"
#define MAIN_MEMORY_SIZE 1048576 /*2^20*/

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

int main(int argc, char* argv[]){
    init_cores();

    /*set instruction memories*/
    char* inst_mems_file_names[NUM_CORES];
    if (argc == 28)
    {
        for (int i = 1; i <= NUM_CORES; i++) {
            inst_mems_file_names[i - 1] = argv[i];
        }
        init_main_memory(argv[5]);
    }
    else
    {
        inst_mems_file_names[0] = "imem0.txt";
        inst_mems_file_names[1] = "imem1.txt";
        inst_mems_file_names[2] = "imem2.txt";
        inst_mems_file_names[3] = "imem3.txt";
        init_main_memory("memin.txt");
        //inst_mems_file_names[5] = "memout.txt";
        //inst_mems_file_names[6] = "regout0.txt";
        //inst_mems_file_names[7] = "regout1.txt";
        //inst_mems_file_names[8] = "regout2.txt";
        //inst_mems_file_names[9] = "regout3.txt";
        //inst_mems_file_names[10] = "core0trace.txt";
        //inst_mems_file_names[11] = "core1trace.txt";
        //inst_mems_file_names[12] = "core2trace.txt";
        //inst_mems_file_names[13] = "core3trace.txt";
        //inst_mems_file_names[14] = "bustrace.txt";
        //inst_mems_file_names[15] = "dsram0.txt";
        //inst_mems_file_names[16] = "dsram1.txt";
        //inst_mems_file_names[17] = "dsram2.txt";
        //inst_mems_file_names[18] = "dsram3.txt";
        //inst_mems_file_names[19] = "tsram0.txt";
        //inst_mems_file_names[20] = "tsram1.txt";
        //inst_mems_file_names[21] = "tsram2.txt";
        //inst_mems_file_names[22] = "tsram3.txt";
        //inst_mems_file_names[23] = "stats0.txt";
        //inst_mems_file_names[24] = "stats1.txt";
        //inst_mems_file_names[25] = "stats2.txt";
        //inst_mems_file_names[26] = "stats3.txt";
    }
    load_inst_mems(inst_mems_file_names);

    /*set main memory*/
    
    run_program(MainMemory);

    //sanity(); /*for debug*/
    /*for(int k=0;k<30;k++){
    	printf("%x, ",MainMemory[k]);
    }*/
    return 0;
}


