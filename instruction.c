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
	if (VERBOSE_MODE) printf("----enter fetch----\n");

	if (core->is_data_stalled)
	{
		uint32_t inst = inst_mem->mem[core->pc];
		core->if_id.D_inst = inst;
		return;
	}

	if (core->fetch == -1) return;

	uint32_t inst = inst_mem->mem[core->pc];
	if (VERBOSE_MODE) printf("pc %d got instruction %x\n",core->pc,inst);
	core->if_id.D_inst = inst;
	//core->pc_prev = core->pc;
	core->next_cycle_pc = core->pc + 1;
	core->next_cycle_fetch = core->next_cycle_pc;
	//core->next_cycle_decode = Active;

	if (VERBOSE_MODE) printf("----exit fetch----\n");


}

int branch_resolution(int opcode, int rs, int rt)
{
	int taken = 0;
	if (opcode == 9 && rs == rt) taken = 1; /*beq*/
	if (opcode == 10 && rs != rt) taken = 1; /*bneq*/
	if (opcode == 11 && rs < rt)  taken = 1; /*blt*/
	if (opcode == 12 && rs > rt)  taken = 1; /*bgt*/
	if (opcode == 13 && rs <= rt) taken = 1; /*ble*/
	if (opcode == 14 && rs >= rt) taken = 1; /*bge*/
	if (opcode == 15)      	                taken = 1; /*jal*/
	return taken;
}

void decode(Core* core, IM* inst_mem){
	if (VERBOSE_MODE) printf("----enter decode----\n");
	if (core->core_id == 2 && clk_cycles == 1)
	{
		printf("");
	}
	if (core->is_data_stalled)
	{
		Instruction* res = (Instruction*)calloc(1, sizeof(Instruction));
		res->imm = 0;
		res->opcode = 0;
		res->rd_index = 0;
		res->rd = 0;
		res->rs = 0;
		res->rt = 0;
		core->id_ex.D_inst = res;
		return;
	}

	if (!core->is_stalled)
	{
		core->next_cycle_decode = core->fetch;
	}

	if (core->decode == -1){
		if (VERBOSE_MODE) printf("not active\n");
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
	if (VERBOSE_MODE) printf("opcode=%d, rd=%d, rs=%d,rt=%d\n",res->opcode, ((inst & 0x00f00000) >> 20),((inst & 0x000f0000) >> 16),((inst & 0x0000f000) >> 12));
	/*Data Hazards*/
	/*check hazards*/
	if((core->regs_to_write[rs_index] == 1) || (core->regs_to_write[rt_index] == 1) || (core->regs_to_write[res->rd_index] ==1 && res->opcode == 17)){ /*hazard detected*/
		if (VERBOSE_MODE) printf("hazard exists\n");
		core->is_stalled = 1;
		core->next_cycle_pc = core->pc;
		core->next_cycle_fetch = core->fetch;
		core->next_cycle_decode = core->decode;
		return;
	}
	else if (core->is_stalled) 
	{
		core->is_stalled = 0;
		core->next_cycle_fetch = Active;
		uint32_t inst = inst_mem->mem[core->decode];
		Instruction* res_stalled = (Instruction*)calloc(1, sizeof(Instruction));
		/*get fields from instruction using masks and shift*/
		res_stalled->imm = (inst & 0x00000fff);
		/*imm goes to reg1*/
		core->regs[1] = res_stalled->imm;

		int rs_index_stalled = (inst & 0x000f0000) >> 16;
		int rt_index_stalled = (inst & 0x0000f000) >> 12;

		res_stalled->opcode = (inst & 0xff000000) >> 24;
		res_stalled->rd_index = (inst & 0x00f00000) >> 20;
		res_stalled->rd = core->regs[(inst & 0x00f00000) >> 20];
		res_stalled->rs = core->regs[rs_index_stalled];
		res_stalled->rt = core->regs[rt_index_stalled];
		if (branch_resolution(res_stalled->opcode, res_stalled->rs, res_stalled->rt))
		{
			core->next_cycle_pc = res_stalled->rd & 0x3ff;
			core->next_cycle_fetch = core->next_cycle_pc;
		}
	}
	if (res->opcode == 20)
	{
		core->next_cycle_fetch = -1;
		core->next_cycle_decode = -1;
		core->halt_pc = core->decode;
	}

	/*set for next cycles*/
	if(res->opcode <= 8 /*arit/log*/ || res->opcode==16 /*lw*/ || res->opcode==18/*ll*/ || res->opcode==19/*sc*/){
		core->regs_to_write[res->rd_index] = 1;
	}
	else if(res->opcode == 15/*jal*/){
		core->regs_to_write[15] = 1;
	}

	if(branch_resolution(res->opcode, res->rs, res->rt)){
		core->next_cycle_pc = res->rd & 0x3ff;
		core->next_cycle_fetch = core->next_cycle_pc;
	}
	core->id_ex.D_inst = res;
	//core->next_cycle_execute = Active;
	if (VERBOSE_MODE) printf("----exit decode----\n");

}

void execute(Core* core){
	if (core->core_id == 2 && clk_cycles == 2)
	{
		printf("");
	}
	if (VERBOSE_MODE) printf("---- enter exectue----\n");
	Instruction* inst = core->id_ex.Q_inst;
	if (core->is_data_stalled)
	{
		core->ex_mem.D_inst = inst;
		return;
	}
	core->next_cycle_execute = core->decode;

	if (core->execute == -1 || core->execute == core->halt_pc){
		if (VERBOSE_MODE) printf("not active\n");
		core->ex_mem.D_inst = inst;
		return;
	}

	if (core->execute == core->decode)
	{
		core->next_cycle_execute = -1;
		core->ex_mem.D_inst = inst;
		return;
	}

	if (VERBOSE_MODE) printf("executing alu op: %d\n",inst->opcode);
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
	if (VERBOSE_MODE) printf("res is: %d\n",core->ex_mem.D_alu_res);
	core->ex_mem.D_inst = inst;
	//core->next_cycle_memory = Active;
	//core->promote_execute_pc = 1;
	if (VERBOSE_MODE) printf("---- exit execute----\n");
}


unsigned short get_set_data_from_memory(Core* core, Instruction* inst, Cache* cache, Bus* bus, uint32_t addr, unsigned short is_get, unsigned short is_sc_cmd)
{
	/* for get enough data in cache. for set must be dirty in cache */
	if (is_data_in_cache(cache, addr))
	{
		if (VERBOSE_MODE) printf("performing cache load/set\n");
		if (is_get == 1)
		{
			inst->rd = get_data_from_cache(cache, addr);
			return 1;
		}
		else
		{
			if (!is_data_in_cache_dirty(cache, addr)) 
			{
				make_BusRdX_request(bus, core->core_id, addr);
			}
			set_data_to_cache(cache, addr, inst->rd, 1);
			if (is_sc_cmd)
			{
				inst->rd = 1;
			}
			return 1;
		}
	}
	else {
		/* the data is ready on the bus */
		if (is_data_ready_from_bus(bus, core->core_id, addr)) {
			if (VERBOSE_MODE) printf("performing bus load/set\n");
			//free_bus_line(bus);
			/* TODO maybe need delay here check with files */
			if (is_get == 1)
			{
				int32_t data = get_data_from_bus(bus);
				inst->rd = data;
				set_data_to_cache(cache, addr, data, 0);
				return 1;
			}
			else
			{
				set_data_to_cache(cache, addr, inst->rd, 1);
				if (is_sc_cmd)
				{
					inst->rd = 1;
				}
				return 1;
			}
		}
		else {
			if (is_bus_free(bus))
			{
				unsigned short curr_cache_index = get_cache_index(addr);
				if (cache->cache[curr_cache_index].state == M)
				{
					unsigned short curr_cache_tag = cache->cache[curr_cache_index].tag;
					uint32_t addr_to_flush = get_main_memory_addr(curr_cache_index, curr_cache_tag);
					int32_t data_to_flush = get_data_from_cache(cache, addr);
					make_Flush_request(bus, core->core_id, addr_to_flush, data_to_flush);
				}
				else if (is_get == 1)
				{
					make_BusRd_request(bus, core->core_id, addr);
				}
				else
				{
					make_BusRdX_request(bus, core->core_id, addr);
				}
			}
			return 0;
		}
	}
}


void memory(Core* core,Cache* cache, Bus* bus, struct WatchFlag** watch){
	if (VERBOSE_MODE) printf("---- enter memory---\n");
	Instruction* inst = core->ex_mem.Q_inst;
	if (!core->is_data_stalled)
	{
		core->next_cycle_memory = core->execute;
	}

	if (core->memory == -1 || core->memory == core->halt_pc){
		if (VERBOSE_MODE) printf("not active\n");
		core->mem_wb.D_inst = inst;
		return;
	}
	int is_stall = 0;
	
	int alu_res = core->ex_mem.Q_alu_res;
	if(inst->opcode == 17 /*sw*/){
		if (VERBOSE_MODE) printf("performing sw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, 0, 0);
	}
	/* put write back value in rd register*/
	else if(inst->opcode == 16 /*lw*/){
		if (VERBOSE_MODE) printf("performing lw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, 1, 0);
	}
	else if(inst->opcode == 18 /*ll*/){
		if (VERBOSE_MODE) printf("performing load linked\n");
		watch[(core->core_id)]->address  = alu_res;
		watch[(core->core_id)]->watching = 1;
		watch[(core->core_id)]->visited  = 0;
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, 1, 0);
	}
	else if(inst->opcode == 19 /*sc*/){
		if (VERBOSE_MODE) printf("performing store conditional\n");
		struct WatchFlag* cur_watch = watch[(core->core_id)];
		if(cur_watch->address == alu_res && cur_watch->watching==1 && cur_watch->visited == 1 ){
			/*sc fail*/
			if (VERBOSE_MODE) printf("sc Failed\n");
			inst->rd = 0;
		}
		else{
			/*sc successes*/
			if (VERBOSE_MODE) printf("sc successes writing to memroy\n");
			is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, 0, 1);
		}
		for(int i=0;i<NUM_CORES;i++){
			if(i == core->core_id) continue;
			if(watch[i]->address == alu_res && watch[i]->watching == 1){
				watch[i]->visited = 1;
				if (VERBOSE_MODE) printf("sc on other core ll\n");
			}
		}

	}

	/*arithmetic/logic op*/
	else if(inst->opcode <= 8){
		if (VERBOSE_MODE) printf("set rd for write back\n");
		inst->rd = alu_res;
	}
	if (is_stall)
	{
		core->is_data_stalled = 1;
		/* stall command */
			//printf("stall for memory read\n");
		core->next_cycle_pc = core->pc;
		//core->if_id.D_inst = core->if_id.Q_inst;
		//core->id_ex.D_inst = core->id_ex.Q_inst;
		//core->ex_mem.D_inst = core->ex_mem.Q_inst;
		//core->ex_mem.D_alu_res = core->ex_mem.Q_alu_res;
		//core->mem_wb.D_inst = nop_inst;
		core->next_cycle_fetch = core->fetch;
		core->next_cycle_decode = core->decode;
		core->next_cycle_execute = core->execute;
		core->next_cycle_memory = core->memory;
		core->next_cycle_writeback = -1;
		
	}
	else
	{
		core->mem_wb.D_inst = inst;
	}

	if (VERBOSE_MODE) printf("----exit memory----\n");
}

int writeBack(Core* core){
	if (VERBOSE_MODE) printf("----enter writeback----\n");

	if (!core->is_data_stalled){
		core->next_cycle_writeback = core->memory;
	}
	
	if (core->writeback == -1)
	{
		return 0;
	}
	else if (core->halt_pc = core->writeback)
	{
		return 1;
	}
	Instruction* inst = core->mem_wb.Q_inst;
	/*perform writeback*/
	if (inst->opcode<=8/*arit/logi*/ || inst->opcode ==16 /*lw*/ || inst->opcode == 18/*ll*/){
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write[inst->rd_index] = 0; /*clean*/
	}
	if(inst->opcode == 15 /*jal*/){
		if (VERBOSE_MODE) printf("writing back jal value %x to reg 15\n",inst->rd);
		core->regs[15] = core->pc-4;
		core->regs_to_write[15] = 0;
	}
	if(inst->opcode == 19 /*sc*/){
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write[inst->rd_index] = 0; /*clean*/
	}
	if(inst->opcode==20/*halt*/){
		if (VERBOSE_MODE) printf("Core Halting\n");
		core->core_state = Halt;
		return Halt;
	}
	free(inst);
	
	if (VERBOSE_MODE) printf("----exit writeback----\n");
	return 0;
}



