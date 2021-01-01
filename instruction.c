/*
 * instruction.c
 *
 *  Created on: 26 Dec 2020
 *      Author: steiy
 */

#include "instruction.h"
#include "cores.h"
#include "cache.h"

void fetch(Core* core,IM* inst_mem){
	printf("----enter fetch----\n");

	if (core->fetch == Stall) return;
	uint32_t inst = inst_mem->mem[core->pc];
	printf("pc %d got instruction %x\n",core->pc,inst);
	core->if_id.D_inst = inst;
	core->pc_prev = core->pc;
	core->pc++;
	core->next_cycle_decode = Active;

	printf("----exit fetch----\n");


}

void decode(Core* core){
	printf("----enter decode----\n");
	if (core->decode == Stall){
		printf("not active\n");
		return;
	}
	uint32_t inst = core->if_id.Q_inst;
	Instruction* res = (Instruction*)calloc(1,sizeof(Instruction));
	/*get fields from instruction using masks and shift*/
	res->imm = (inst & 0x00000fff);
	/*imm goes to reg1*/
	core->regs[1] = res->imm;

	int rs_index = (inst & 0x000f0000) >> 16;
	int rt_index = (inst & 0x0000f000) >> 12;

	res->opcode = (inst & 0xff000000) >> 24;
	res->rd_index = (inst & 0x00f00000) >> 20;
	res->rd = core->regs[(inst & 0x00f00000) >> 20];
	res->rs = core->regs[rs_index];
	res->rt = core->regs[rt_index];
	printf("opcode=%d, rd=%d, rs=%d,rt=%d\n",res->opcode, ((inst & 0x00f00000) >> 20),((inst & 0x000f0000) >> 16),((inst & 0x0000f000) >> 12));
	/*Data Hazards*/
	/*check hazards*/
	if((core->regs_to_write[rs_index] == 1) || (core->regs_to_write[rt_index] == 1) || (core->regs_to_write[res->rd_index] ==1 && res->opcode == 17)){ /*hazard detected*/
		printf("hazard exists\n");
		core->pc= core->pc_prev;
		core->if_id.D_inst = core->if_id.Q_inst;
		Instruction* nop_inst = (Instruction*)calloc(1,sizeof(Instruction));
		nop_inst->opcode = -1;
		core->id_ex.D_inst = nop_inst;
		return;
	}
	else{
		core->next_cycle_fetch = Active;
	}

	/*set for next cycles*/
	if(res->opcode <= 8 /*arit/log*/ || res->opcode==16 /*lw*/ || res->opcode==18/*ll*/ || res->opcode==19/*sc*/){
		core->regs_to_write[res->rd_index] = 1;
	}
	if(res->opcode == 15/*jal*/){
		core->regs_to_write[15] = 1;
	}

	/*Branch Resolution*/
	if(res->opcode <= 14 && 9 <= res->opcode){
		int taken = 0;
		if(res->opcode == 9  && res->rs == res->rt) taken=1; /*beq*/
		if(res->opcode == 10 && res->rs != res->rt) taken=1; /*bneq*/
		if(res->opcode == 11 && res->rs < res->rt)  taken=1; /*blt*/
		if(res->opcode == 12 && res->rs > res->rt)  taken=1; /*bgt*/
		if(res->opcode == 13 && res->rs <= res->rt) taken=1; /*ble*/
		if(res->opcode == 14 && res->rs >= res->rt) taken=1; /*bge*/
		if(res->opcode == 15)      	                taken=1; /*jal*/

		if(taken == 1){
			core->pc_prev = core->pc;
			core->pc = (res->rd & 0x3ff);
		}
	}
	core->id_ex.D_inst = res;
	core->next_cycle_execute = Active;
	printf("----exit decode----\n");

}
void execute(Core* core){
	printf("---- enter exectue----\n");
	if (core->execute == Stall){
		printf("not active\n");
		return;
	}
	Instruction* inst = core->id_ex.Q_inst;
	if (inst->opcode == -1){/*nop*/
		core->ex_mem.D_inst = inst;
		return;
	}
	printf("executing alu op: %d\n",inst->opcode);
	switch(inst->opcode){
	case(0):/*add*/
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(1):/*sub*/
		core->ex_mem.D_alu_res = inst->rs - inst->rt;
		break;
	case(2):/*and*/
		core->ex_mem.D_alu_res = inst->rs & inst->rt;
		break;
	case(3):/*or*/
		core->ex_mem.D_alu_res = inst->rs | inst->rt;
		break;
	case(4):/*xor*/
		core->ex_mem.D_alu_res = inst->rs ^ inst->rt;
		break;
	case(5):/*mult*/
		core->ex_mem.D_alu_res = inst->rs * inst->rt;
		break;
	case(6):/*sll*/
		core->ex_mem.D_alu_res = (inst->rs) << inst->rt;
		break;
	case(7):/*sra*/
		core->ex_mem.D_alu_res = (inst->rs) >> inst->rt;
		break;
	case(8):/*srl*/
		core->ex_mem.D_alu_res = ((inst->rs) >> inst->rt) & ~(1<<31);
		break;
	case(16):/*lw*/
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(17):/*sw*/
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(18):/*ll*/
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(19):/*sc*/
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	}
	printf("res is: %d\n",core->ex_mem.D_alu_res);
	core->ex_mem.D_inst = inst;
	core->next_cycle_memory = Active;
	printf("---- exit execute----\n");
}


void get_data_from_memory(Core* core, Instruction* inst, Cache* cache, uint32_t* MM, Bus* bus, uint32_t addr)
{
	if (is_data_in_cache(cache, addr))
	{
		printf("performing cache load\n");
		inst->rd = get_data_from_cache(cache, addr);
	}
	else {
		/* the data is ready on the bus */
		if (is_data_ready_from_bus(bus, addr)) {
			printf("performing bus load\n");
			int32_t data = get_data_from_bus(bus);
			inst->rd = data;
			free_bus_line(bus);
			set_data_to_cache(cache, addr, data, 0);
		}
		else {
			if (is_bus_free(bus))
			{
				printf("requesting busrd\n");
				make_BusRd_request(bus, core->core_id, addr);
			}
			/* stall command */
			printf("stall for memory read\n");
			core->pc = core->pc_prev;
			core->if_id.D_inst = core->if_id.Q_inst;
			Instruction* nop_inst = (Instruction*)calloc(1, sizeof(Instruction));
			nop_inst->opcode = -1;
			core->id_ex.D_inst = nop_inst;

			core->if_id.Q_inst = core->if_id.D_inst;
			core->id_ex.Q_inst = core->id_ex.D_inst;
			core->ex_mem.Q_inst = core->ex_mem.D_inst;
			core->ex_mem.Q_alu_res = core->ex_mem.D_alu_res;
			core->mem_wb.Q_inst = core->mem_wb.D_inst;

			core->fetch = core->next_cycle_fetch;
			core->decode = core->next_cycle_decode;
			core->execute = core->next_cycle_execute;
			core->memory = core->next_cycle_memory;
			core->writeback = core->next_cycle_writeback;
		}


	}
}


void memory(Core* core,Cache* cache, uint32_t* MM, Bus* bus, struct WatchFlag** watch){
	printf("---- enter memory---\n");
	if (core->memory == Stall){
		printf("not active\n");
		return;
	}
	Instruction* inst = core->ex_mem.Q_inst;
	if (inst->opcode == -1){/*nop*/
			core->mem_wb.D_inst = inst;
			return;
		}
	int alu_res = core->ex_mem.Q_alu_res;
	if(inst->opcode == 17 /*sw*/){
		printf("performing MM sw\n");
		MM[alu_res] = inst->rd;
	}
	/* put write back value in rd register*/
	if(inst->opcode == 16 /*lw*/){
		printf("performing load word\n");
		get_data_from_memory(core, inst, cache, MM, bus, alu_res);	
	}
	if(inst->opcode == 18 /*ll*/){
		printf("performing load linked\n");
		get_data_from_memory(core, inst, cache, MM, bus, alu_res);
		watch[(core->core_id)]->address  = alu_res;
		watch[(core->core_id)]->watching = 1;
		watch[(core->core_id)]->visited  = 0;
	}
	if(inst->opcode == 19 /*sc*/){
		printf("performing MM store conditional\n");
		struct WatchFlag* cur_watch = watch[(core->core_id)];
		if(cur_watch->address == alu_res && cur_watch->watching==1 && cur_watch->visited == 1 ){
			/*sc fail*/
			printf("sc Failed\n");
			inst->rd = 0;
		}
		else{
			/*sc successes*/
			printf("sc successes writing to memroy\n");
			MM[alu_res] = inst->rd;
			inst->rd = 1;
		}
		for(int i=0;i<NUM_CORES;i++){
			if(i == core->core_id) continue;
			if(watch[i]->address == alu_res && watch[i]->watching == 1){
				watch[i]->visited = 1;
				printf("sc on other core ll\n");
			}
		}

	}

	/*arithmetic/logic op*/
	if(inst->opcode <= 8){
		printf("set rd for write back\n");
		inst->rd = alu_res;
	}
	core->mem_wb.D_inst = inst;
	core->next_cycle_writeback = Active;

	printf("----exit memory----\n");
}
enum State writeBack(Core* core){
	printf("----enter writeback----\n");

	if (core->writeback == Stall){
		printf("not active\n");
		/*signal that core itself is still active*/
		return Active;
	}
	Instruction* inst = core->mem_wb.Q_inst;
	if (inst->opcode == -1){/*nop*/
		free(inst);
		return Active;
	}
	/*perform writeback*/
	if (inst->opcode<=8/*arit/logi*/ || inst->opcode ==16 /*lw*/ || inst->opcode == 18/*ll*/){
		printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
				core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write[inst->rd_index] = 0; /*clean*/
	}
	if(inst->opcode == 15 /*jal*/){
		printf("writing back jal value %x to reg 15\n",inst->rd);
		core->regs[15] = core->pc-4;
		core->regs_to_write[15] = 0;
	}
	if(inst->opcode == 19 /*sc*/){
		printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write[inst->rd_index] = 0; /*clean*/
	}
	if(inst->opcode==20/*halt*/){
		printf("Core Halting\n");
		core->core_state = Halt;
		return Halt;
	}
	free(inst);
	return Active;
	printf("----exit writeback----\n");

}



