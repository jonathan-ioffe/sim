#include <stdio.h>
#include <stdlib.h>
#include "cores.h"
#include "instruction.h"
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
    int i;
    for (i=1; i<=NUM_CORES; i++){
    	inst_mems_file_names[i-1] = argv[i];
    }
    load_inst_mems(inst_mems_file_names);

    /*set main memory*/
    init_main_memory(argv[i++]);
    run_program(MainMemory);

    //sanity(); /*for debug*/
    /*for(int k=0;k<30;k++){
    	printf("%x, ",MainMemory[k]);
    }*/
    return 0;
}


