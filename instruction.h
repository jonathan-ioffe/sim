/*
 * instruction.h
 *
 *  Created on: 26 Dec 2020
 *      Author: steiy
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include "cores.h"
#include "bus.h"

#define NOP_OP -1
#define ADD_OP 0
#define SUB_OP 1
#define AND_OP 2
#define OR_OP 3
#define XOR_OP 4
#define MUL_OP 5
#define SLL_OP 6
#define SRA_OP 7
#define SRL_OP 8
#define BEQ_OP 9
#define BNE_OP 10
#define BLT_OP 11
#define BGT_OP 12
#define BLE_OP 13
#define BGE_OP 14
#define JAL_OP 15
#define LW_OP 16
#define SW_OP 17
#define LL_OP 18
#define SC_OP 19
#define HALT_OP 20
#define NOT_INITIALIZED -1

void fetch(Core* core,IM* inst_mem);
void decode(Core* core);
void execute(Core* core);
void memory(Core* core, Cache* cache, struct WatchFlag** watch);
bool write_back(Core* core);

#endif /* INSTRUCTION_H_ */
