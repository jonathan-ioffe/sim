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
	bool visited;
	bool watching;
};

enum State {Active, Stall, Halt};
enum BusReadState {Free, MadeRd, MadeRdX, MadeFlush, Pending, Ready};

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
    bool regs_to_write_D[NUM_REGS];
    bool regs_to_write_Q[NUM_REGS];
    int regs_to_write_pc_invoked[NUM_REGS];
    enum State core_state_Q;
    enum State core_state_D;
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
    uint32_t fetch_pc_Q, fetch_pc_D;
    uint32_t decode_pc_Q, decode_pc_D;
    uint32_t execute_pc_Q, execute_pc_D;
    uint32_t memory_pc_Q, memory_pc_D;
    uint32_t writeback_pc_Q, writeback_pc_D;
    uint32_t halt_pc;

 /*   enum BusReadState pending_bus_read_D, pending_bus_read_Q;
    uint32_t pending_bus_read_addr_D, pending_bus_read_addr_Q;
    int32_t pending_bus_read_data_D, pending_bus_read_data_Q;*/
    enum BusReadState pending_bus_read;
    uint32_t pending_bus_read_addr;
    int32_t pending_bus_read_data;

    bool is_data_stall;
    bool is_data_hazard;
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
