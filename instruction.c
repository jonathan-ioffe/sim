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

	if (core->fetch == Stall) {
		return;
	}

	uint32_t inst = inst_mem->mem[core->pc];
	if (VERBOSE_MODE) printf("pc %d got instruction %x\n",core->pc,inst);
	core->if_id.D_inst = inst;
	// count instruction only if fetched a new instruction. if already halted stop counting
	if (core->pc_prev != core->pc && core->halt_pc == -1) core->core_stats_counts.instructions++; 
	core->pc_prev = core->pc;
	core->pc++;
	core->next_cycle_decode = Active;

	core->decode_pc_D = core->fetch_pc_Q;

	if (VERBOSE_MODE) printf("----exit fetch----\n");


}

void decode(Core* core){
	if (VERBOSE_MODE) printf("----enter decode----\n");
	if (core->decode == Stall)
	{
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
	if((core->regs_to_write_Q[rs_index]) || (core->regs_to_write_Q[rt_index]) || (core->regs_to_write_Q[res->rd_index] && res->opcode == 17)){ /*hazard detected*/
		if (VERBOSE_MODE) printf("hazard exists\n");
		if (!core->is_data_stall) core->core_stats_counts.decode_stall++; // count decode stall only if it's not stalling because of memory hazard before
		core->pc= core->pc_prev;
		core->if_id.D_inst = core->if_id.Q_inst;
		Instruction* nop_inst = (Instruction*)calloc(1,sizeof(Instruction));
		nop_inst->opcode = -1;
		core->id_ex.D_inst = nop_inst;
		core->is_data_hazard = true;
		core->decode_pc_D = core->decode_pc_Q;
		core->execute_pc_D = core->execute_pc_Q;
		//core->memory_pc_D = core->memory_pc_Q;
		//core->writeback_pc_D = core->writeback_pc_Q;
		return;
	}
	else
	{
		core->next_cycle_fetch = Active;
		core->is_data_hazard = false;
		//core->decode_pc_D = core->fetch_pc_Q;
		core->execute_pc_D = core->decode_pc_Q;
	}

	/*set for next cycles*/
	if(res->opcode <= 8 /*arit/log*/ || res->opcode==16 /*lw*/ || res->opcode==18/*ll*/ || res->opcode==19/*sc*/){
		core->regs_to_write_D[res->rd_index] = true;
	}
	if(res->opcode == 15/*jal*/){
		core->regs_to_write_D[15] = true;
	}

	/*Branch Resolution*/
	if(res->opcode <= 14 && 9 <= res->opcode){
		bool taken = false;
		if(res->opcode == 9  && res->rs == res->rt) taken=true; /*beq*/
		if(res->opcode == 10 && res->rs != res->rt) taken=true; /*bneq*/
		if(res->opcode == 11 && res->rs < res->rt)  taken=true; /*blt*/
		if(res->opcode == 12 && res->rs > res->rt)  taken=true; /*bgt*/
		if(res->opcode == 13 && res->rs <= res->rt) taken=true; /*ble*/
		if(res->opcode == 14 && res->rs >= res->rt) taken=true; /*bge*/
		if(res->opcode == 15)      	                taken=true; /*jal*/

		if(taken)
		{
			core->pc_prev = core->pc;
			core->pc = (res->rd & 0x3ff);
		}
	}
	/* Detect halt */
	if (res->opcode == 20 && core->halt_pc == -1)
	{
		core->halt_pc = core->decode_pc_Q+1; // let the first halt propagte in the trace
	}
	core->id_ex.D_inst = res;
	core->next_cycle_execute = Active;
	if (VERBOSE_MODE) printf("----exit decode----\n");

}

void execute(Core* core){
	if (VERBOSE_MODE) printf("---- enter exectue----\n");
	if (core->execute == Stall){
		if (VERBOSE_MODE) printf("not active\n");
		return;
	}
	Instruction* inst = core->id_ex.Q_inst;
	if (inst->opcode == -1){/*nop*/
		core->ex_mem.D_inst = inst;
		/*core->execute_pc_D = core->execute_pc_Q;*/
		return;
	}
	core->memory_pc_D = core->execute_pc_Q;
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
	core->next_cycle_memory = Active;
	if (VERBOSE_MODE) printf("---- exit execute----\n");
}

bool get_set_data_from_memory(Core* core, Instruction* inst, Cache* cache, Bus* bus, uint32_t addr, bool is_get, bool is_sc_cmd)
{
	/* for get enough data in cache. for set must be dirty in cache */
	if (is_data_in_cache(cache, addr))
	{
		if (VERBOSE_MODE) printf("performing cache load/set\n");
		if (is_get)
		{
			inst->rd = get_data_from_cache(cache, addr);
			core->core_stats_counts.read_hit++;
			return true;
		}
		else
		{
			if (!is_data_in_cache_dirty(cache, addr)) 
			{
				if (!core->is_data_stall && core->pending_bus_read == Free)
				{
					core->pending_bus_read = MadeRdX;
					core->pending_bus_read_addr = addr;
					invalidate_cache_addr(cache, addr);
				}
				//if (!core->is_data_stall && is_bus_free(bus)) make_BusRdX_request(bus, core->core_id, addr);
				core->core_stats_counts.write_miss++;
				return false;
			}
			set_data_to_cache(cache, addr, inst->rd, 1);
			if (is_sc_cmd)
			{
				inst->rd = 1;
			}
			core->core_stats_counts.write_hit++;
			return true;
		}
	}
	else {
		/* the data is ready on the bus */
		/*if (is_data_ready_from_bus(bus, core->core_id, addr)) {*/
		if (core->pending_bus_read == Ready) {
			if (VERBOSE_MODE) printf("performing bus load/set\n");
			core->is_data_stall = false;
			core->pending_bus_read = Free;
			//free_bus_line(bus);
			/* TODO maybe need delay here check with files */
			if (is_get)
			{
				//int32_t data = get_data_from_bus(bus);
				int32_t data = core->pending_bus_read_data;
				inst->rd = data;
				set_data_to_cache(cache, addr, data, 0);
			}
			else
			{
				set_data_to_cache(cache, addr, inst->rd, 1);
				if (is_sc_cmd)
				{
					inst->rd = 1;
				}				
			}
			return true;
		}
		else {
			if (!core->is_data_stall && core->pending_bus_read == Free)
			{
				unsigned short curr_cache_index = get_cache_index(addr);
				if (cache->cache[curr_cache_index].state == M)
				{
					unsigned short curr_cache_tag = cache->cache[curr_cache_index].tag;
					uint32_t addr_to_flush = get_main_memory_addr(curr_cache_index, curr_cache_tag);
					int32_t data_to_flush = get_data_from_cache(cache, addr);
					core->pending_bus_read = MadeFlush;
					core->pending_bus_read_addr = addr_to_flush;
					core->pending_bus_read_data = data_to_flush;
					invalidate_cache_addr(cache, addr_to_flush);
					//make_Flush_request(bus, core->core_id, addr_to_flush, data_to_flush);
				}
				else if (is_get)
				{
					//if (!core->is_data_stall) make_BusRd_request(bus, core->core_id, addr);
					core->pending_bus_read = MadeRd;
					core->pending_bus_read_addr = addr;
					core->core_stats_counts.read_miss++;
				}
				else
				{
					//if (!core->is_data_stall) make_BusRdX_request(bus, core->core_id, addr);
					core->pending_bus_read = MadeRdX;
					core->pending_bus_read_addr = addr;
					core->core_stats_counts.write_miss++;
				}
			}
			/*if (is_bus_free(bus))
			{
				unsigned short curr_cache_index = get_cache_index(addr);
				if (cache->cache[curr_cache_index].state == M)
				{
					unsigned short curr_cache_tag = cache->cache[curr_cache_index].tag;
					uint32_t addr_to_flush = get_main_memory_addr(curr_cache_index, curr_cache_tag);
					int32_t data_to_flush = get_data_from_cache(cache, addr);
					make_Flush_request(bus, core->core_id, addr_to_flush, data_to_flush);
				}
				else if (is_get)
				{
					if (!core->is_data_stall) make_BusRd_request(bus, core->core_id, addr);
				}
				else
				{
					if (!core->is_data_stall) make_BusRdX_request(bus, core->core_id, addr);
				}
			}*/
			return false;
		}
	}
}

void memory(Core* core,Cache* cache, Bus* bus, struct WatchFlag** watch){
	if (VERBOSE_MODE) printf("---- enter memory---\n");
	if (core->memory == Stall){
		if (VERBOSE_MODE) printf("not active\n");
		return;
	}
	bool is_stall = false;
	Instruction* inst = core->ex_mem.Q_inst;
	if (inst->opcode == -1)
	{/*nop*/
			core->mem_wb.D_inst = inst;
			return;
	}
	int alu_res = core->ex_mem.Q_alu_res;
	if(inst->opcode == 17 /*sw*/){
		if (VERBOSE_MODE) printf("performing sw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, false, false);
	}
	/* put write back value in rd register*/
	else if(inst->opcode == 16 /*lw*/){
		if (VERBOSE_MODE) printf("performing lw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, true, false);
	}
	else if(inst->opcode == 18 /*ll*/){
		if (VERBOSE_MODE) printf("performing load linked\n");
		watch[(core->core_id)]->address  = alu_res;
		watch[(core->core_id)]->watching = 1;
		watch[(core->core_id)]->visited  = 0;
		is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, true, false);
	}
	else if(inst->opcode == 19 /*sc*/){
		if (VERBOSE_MODE) printf("performing store conditional\n");
		struct WatchFlag* cur_watch = watch[(core->core_id)];
		if(cur_watch->address == alu_res && cur_watch->watching && cur_watch->visited)
		{
			/*sc fail*/
			if (VERBOSE_MODE) printf("sc Failed\n");
			inst->rd = 0;
		}
		else{
			/*sc successes*/
			if (VERBOSE_MODE) printf("sc successes writing to memroy\n");
			is_stall = !get_set_data_from_memory(core, inst, cache, bus, alu_res, false, true);
		}
		for(int i=0;i<NUM_CORES;i++){
			if(i == core->core_id) continue;
			if(watch[i]->address == alu_res && watch[i]->watching)
			{
				watch[i]->visited = true;
				if (VERBOSE_MODE) printf("sc on other core ll\n");
			}
		}


	}
	/*arithmetic/logic op*/
	else if(inst->opcode <= 8){
		if (VERBOSE_MODE) printf("set rd for write back\n");
		inst->rd = alu_res;
	}
	if (!is_stall)
	{
		core->mem_wb.D_inst = inst;
		core->writeback_pc_D = core->memory_pc_Q;
	}
	else
	{
		core->is_data_stall = true;
		core->core_stats_counts.mem_stall++;
		/* stall command */
			//printf("stall for memory read\n");
		core->pc = core->pc_prev;
		/*Instruction* nop_inst = (Instruction*)calloc(1, sizeof(Instruction));
		nop_inst->opcode = -1;*/
		core->if_id.D_inst = core->if_id.Q_inst;
		core->id_ex.D_inst = core->id_ex.Q_inst;
		core->ex_mem.D_inst = core->ex_mem.Q_inst;
		core->ex_mem.D_alu_res = core->ex_mem.Q_alu_res;
		core->mem_wb.D_inst = (Instruction*)calloc(1, sizeof(Instruction));
		core->mem_wb.D_inst->opcode = -1;
		core->next_cycle_fetch = core->fetch;
		core->next_cycle_decode = core->decode;
		core->next_cycle_execute = core->execute;
		core->next_cycle_memory = core->memory;
		core->next_cycle_writeback = core->writeback;
		core->writeback_pc_D = core->writeback_pc_Q;
		core->memory_pc_D = core->memory_pc_Q;
		core->execute_pc_D = core->execute_pc_Q;
		core->decode_pc_D = core->decode_pc_Q;
		for (int i = 0; i < NUM_REGS; i++)
		{
			core->regs_to_write_D[i] = core->regs_to_write_Q[i];
		}
	}
	core->next_cycle_writeback = Active;

	if (VERBOSE_MODE) printf("----exit memory----\n");
}

enum State writeBack(Core* core){
	if (VERBOSE_MODE) printf("----enter writeback----\n");

	if (core->writeback == Stall){
		if (VERBOSE_MODE) printf("not active\n");
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
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write_D[inst->rd_index] = false; /*clean*/
	}
	if(inst->opcode == 15 /*jal*/){
		if (VERBOSE_MODE) printf("writing back jal value %x to reg 15\n",inst->rd);
		core->regs[15] = core->pc-4;
		core->regs_to_write_D[15] = false;
	}
	if(inst->opcode == 19 /*sc*/){
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index!=0){
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write_D[inst->rd_index] = false; /*clean*/
	}
	if(inst->opcode==20/*halt*/){
		if (VERBOSE_MODE) printf("Core Halting\n");
		core->core_state_D = Halt;
		return Halt;
	}
	free(inst);
	if (core->core_id == 1 && clk_cycles == 5)
	{
		printf("");
	}
	if (core->memory_pc_Q == core->writeback_pc_Q || core->memory_pc_D == core->writeback_pc_D)
	{
		core->memory_pc_D = -1;
	}
	if (core->execute_pc_Q == core->writeback_pc_Q || core->execute_pc_D == core->writeback_pc_D)
	{
		core->execute_pc_D = -1;
	}
	if (core->decode_pc_Q == core->writeback_pc_Q || core->decode_pc_D == core->writeback_pc_D)
	{
		core->decode_pc_D = -1;
	}
	if (core->writeback_pc_D == core->writeback_pc_Q)
	
	{
		core->writeback_pc_D = -1;
	}
	if (VERBOSE_MODE) printf("----exit writeback----\n");
	return Active;
}



