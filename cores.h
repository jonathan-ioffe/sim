/*
 * cores.h
 *
 *  Created on: 25 Dec 2020
 *      Author: steiy
 */

#ifndef CORES_H_
#define CORES_H_
#include "main.h"

#define NUM_REGS 16
#define NUM_CORES 4
#define IM_SIZE 1024
#define CACHE_SIZE 256
#define HEX_INST_LEN  10
#define MAIN_MEMORY_FETCH_DELAY 64
#define BUS_FETCH_DELAY 64
#include "bus.h"
typedef struct instruction{
	int opcode;
	uint32_t rd;
	uint32_t rs;
	uint32_t rt;
	int rd_index;
	int imm;
}Instruction;

struct WatchFlag{
	uint32_t address;
	int visited;
	int watching;
};

enum State {Active, Stall, Halt};

struct IF_ID_Reg{
	uint32_t D_inst;
	uint32_t Q_inst;
};

struct ID_EX_Reg{
	Instruction* D_inst;
	Instruction* Q_inst;
};
struct EX_MEM_Reg{
	Instruction* D_inst;
	Instruction* Q_inst;
	uint32_t D_alu_res;
	uint32_t Q_alu_res;

};
struct MEM_WB_Reg{
	Instruction* D_inst;
	Instruction* Q_inst;
};


typedef struct IM{
    uint32_t mem[IM_SIZE];
}IM;


typedef struct core{
    int pc;
    int next_cycle_pc;
    int core_id;
    uint32_t regs[NUM_REGS];
    /*pipeline structs*/
    struct IF_ID_Reg if_id;
    struct ID_EX_Reg id_ex;
    struct EX_MEM_Reg ex_mem;
    struct MEM_WB_Reg mem_wb;
    int regs_to_write[NUM_REGS];
    enum State core_state;
    int fetch;
    int decode;
    int execute;
    int memory;
    int writeback;
    int next_cycle_fetch;
    int next_cycle_decode;
    int next_cycle_execute;
    int next_cycle_memory;
    int next_cycle_writeback;

    int is_data_stalled;
    int is_stalled;
    int halt_pc;



}Core;

enum MSI{I/*Invalid*/, S/*Shared*/, M/*Modified*/};


struct cache_line{
    /* DSRAM */
	uint32_t data;
    /* TSRAM */
	enum MSI state;
	int tag; /*12 bit tag*/
};

typedef struct cache{
	struct cache_line cache[CACHE_SIZE];
}Cache;



void init_cores(char** core_trace_file_names);
void init_bus(char* bus_trace_file_name);
void sanity(); /*for debug*/
void load_inst_mems(char** inst_mems_file_names);
void write_core_regs_files(char** regout_file_names);
void run_program(uint32_t* MM);


#endif /* CORES_H_ */
