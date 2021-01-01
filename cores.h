/*
 * cores.h
 *
 *  Created on: 25 Dec 2020
 *      Author: steiy
 */

#ifndef CORES_H_
#define CORES_H_
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

#include <stdlib.h>
#include <stdio.h>
#define NUM_REGS 16
#define NUM_CORES 4
#define IM_SIZE 1024
#define CACHE_SIZE 256
#define HEX_INST_LEN  10
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
    int pc_prev;
    int core_id;
    uint32_t regs[NUM_REGS];
    /*pipeline structs*/
    struct IF_ID_Reg if_id;
    struct ID_EX_Reg id_ex;
    struct EX_MEM_Reg ex_mem;
    struct MEM_WB_Reg mem_wb;
    int regs_to_write[NUM_REGS];
    enum State core_state;
    enum State fetch;
    enum State decode;
    enum State execute;
    enum State memory;
    enum State writeback;
    enum State next_cycle_fetch;
    enum State next_cycle_decode;
    enum State next_cycle_execute;
    enum State next_cycle_memory;
    enum State next_cycle_writeback;



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



void init_cores();
void sanity(); /*for debug*/
void load_inst_mems();
void run_program(uint32_t* MM, Bus* bus);


#endif /* CORES_H_ */
