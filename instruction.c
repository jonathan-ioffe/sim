/*
 * instruction.c
 *
 *  Created on: 26 Dec 2020
 *      Author: steiy
 */

#include "instruction.h"
#include "cores.h"
#include "cache.h"

/* functions to get the different parts of the instruction */
int get_opcode(uint32_t inst)
{
	return (inst & 0xff000000) >> 24;
}
int get_imm(uint32_t inst)
{
	return inst & 0x00000fff;
}
int get_rs_idx(uint32_t inst)
{
	return (inst & 0x000f0000) >> 16;
}
int get_rt_idx(uint32_t inst)
{
	return (inst & 0x0000f000) >> 12;
}
int get_rd_idx(uint32_t inst)
{
	return (inst & 0x00f00000) >> 20;
}

// check if branch is taken and update the pc if needed
void branch_resolution(Core* core, Instruction* inst)
{
	bool taken = false;
	// check if the branch should be taken
	switch (inst->opcode)
	{
	case(BEQ_OP):
		taken = inst->rs == inst->rt;
		break;
	case(BNE_OP):
		taken = inst->rs != inst->rt;
		break;
	case(BLT_OP):
		taken = inst->rs < inst->rt;
		break;
	case(BGT_OP):
		taken = inst->rs > inst->rt;
		break;
	case(BLE_OP):
		taken = inst->rs <= inst->rt;
		break;
	case(BGE_OP):
		taken = inst->rs >= inst->rt;
		break;
	case(JAL_OP):
		taken = true;
		break;
	}

	if (taken)
	{
		// update the next pc to be the branch jump
		core->pc_prev = core->pc;
		core->pc = (inst->rd & 0x3ff);
	}
	
}
// handle get/set data from memory (cache/bus). return true if could get it successfully
bool get_set_data_from_memory(Core* core, Instruction* inst, Cache* cache, uint32_t addr, bool is_get, bool is_sc_cmd)
{
	if (is_data_in_cache(cache, addr))
	{
		if (VERBOSE_MODE) printf("performing cache load/set\n");
		if (is_get)
		{
			// found the data in cache for get op, use it
			inst->rd = get_data_from_cache(cache, addr);
			core->core_stats_counts.read_hit++;
			return true;
		}
		else
		{
			if (!is_data_in_cache_dirty(cache, addr))
			{
				// found data in cache for set op, but it's not dirty so ask for it if free to ask
				if (!core->is_data_stall && core->pending_bus_read == Free)
				{
					core->pending_bus_read = MadeRdX;
					core->pending_bus_read_addr = addr;
					invalidate_cache_addr(cache, addr);
				}
				core->core_stats_counts.write_miss++;
				return false;
			}
			// found dirty data in cache for set op, use it
			set_data_to_cache(cache, addr, inst->rd, true);
			if (is_sc_cmd)
			{
				inst->rd = 1;
			}
			core->core_stats_counts.write_hit++;
			return true;
		}
	}
	else 
	{
		// data not in cache at all
		if (core->pending_bus_read == Ready) 
		{
			// the data is ready on the bus
			if (VERBOSE_MODE) printf("performing bus load/set\n");
			core->is_data_stall = false;
			core->pending_bus_read = Free;
			if (is_get)
			{
				int32_t data = core->pending_bus_read_data;
				inst->rd = data;
				set_data_to_cache(cache, addr, data, false);
			}
			else
			{
				set_data_to_cache(cache, addr, inst->rd, true);
				if (is_sc_cmd)
				{
					inst->rd = 1;
				}
			}
			return true;
		}
		else {
			// the data is not on the bus yet
			if (core->pending_bus_read == Free)
			{
				// not stalled already and the can ask from bus
				unsigned short curr_cache_index = get_cache_index(addr);
				
				if (cache->cache[curr_cache_index].state == M)
				//if (false)
				{
					// there's dirty data in cache (from other tag). flush it
					unsigned short curr_cache_tag = cache->cache[curr_cache_index].tag;
					uint32_t addr_to_flush = get_main_memory_addr(curr_cache_index, curr_cache_tag);
					int32_t data_to_flush = get_data_from_cache(cache, addr);
					core->pending_bus_read = MadeFlush;
					core->pending_bus_read_addr = addr_to_flush;
					core->pending_bus_read_data = data_to_flush;
					invalidate_cache_addr(cache, addr_to_flush);
				}
				else if (is_get)
				{
					// ask for regular read from bus
					core->pending_bus_read = MadeRd;
					core->pending_bus_read_addr = addr;
					core->core_stats_counts.read_miss++;
				}
				else
				{
					// ask for exculsive read from bus
					core->pending_bus_read = MadeRdX;
					core->pending_bus_read_addr = addr;
					core->core_stats_counts.write_miss++;
				}
			}
			return false;
		}
	}
}

void fetch(Core* core,IM* inst_mem){
	if (VERBOSE_MODE) printf("----enter fetch----\n");

	uint32_t inst = inst_mem->mem[core->pc]; // get instruction from memory
	if (VERBOSE_MODE) printf("pc %d got instruction %08X\n",core->pc,inst);
	core->if_id.D_inst = inst;
	// count instruction only if fetched a new instruction. if already halted stop counting
	if (core->pc_prev != core->pc && core->halt_pc == NOT_INITIALIZED) core->core_stats_counts.instructions++; 

	// update pc count
	core->pc_prev = core->pc;
	core->pc++;

	// update for next step in pipe
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

	// get the current instruction and decode it
	uint32_t inst_raw = core->if_id.Q_inst;
	Instruction* inst = (Instruction*)calloc(1,sizeof(Instruction));
	/*get fields from instruction using masks and shift*/
	inst->imm = get_imm(inst_raw);
	/*imm goes to reg1*/
	core->regs[IMM_REG_IDX] = inst->imm;

	int rs_index = get_rs_idx(inst_raw);
	int rt_index = get_rt_idx(inst_raw);
	int rd_index = get_rd_idx(inst_raw);
	
	inst->opcode = get_opcode(inst_raw);
	inst->rd_index = rd_index;
	inst->rd = core->regs[rd_index];
	inst->rs = core->regs[rs_index];
	inst->rt = core->regs[rt_index];
	if (VERBOSE_MODE) printf("opcode=%d, rd=%d, rs=%d,rt=%d\n", inst->opcode, rd_index, rs_index, rt_index);
	/*check hazards*/
	if((core->regs_to_write_Q[rs_index]) || (core->regs_to_write_Q[rt_index]) || (core->regs_to_write_Q[inst->rd_index] && inst->opcode == SW_OP))
	{ /*hazard detected*/
		handle_data_hazard(core);
		return;
	}
	else
	{
		// report no data hazard
		core->is_data_hazard = false;
		// update for next step in pipe
		core->execute_pc_D = core->decode_pc_Q;
	}

	// mark which registers will be used to block for data hazard
	if(inst->opcode <= SRL_OP /*arit/log*/ || inst->opcode == LW_OP || inst->opcode==LL_OP || inst->opcode==SC_OP)
	{
		core->regs_to_write_D[inst->rd_index] = true;
	}
	if(inst->opcode == JAL_OP)
	{
		core->regs_to_write_D[JAL_REG_IDX] = true;
	}

	branch_resolution(core, inst);
	/* Detect halt */
	if (inst->opcode == HALT_OP && core->halt_pc == NOT_INITIALIZED)
	{
		core->halt_pc = core->decode_pc_Q+1; // let the first halt propagte in the trace
	}
	// update for next step in pipe
	core->id_ex.D_inst = inst;
	core->next_cycle_execute = Active;
	if (VERBOSE_MODE) printf("----exit decode----\n");

}
void execute(Core* core){
	if (VERBOSE_MODE) printf("---- enter exectue----\n");
	if (core->execute == Stall)
	{
		if (VERBOSE_MODE) printf("not active\n");
		return;
	}
	Instruction* inst = core->id_ex.Q_inst;
	// dont do anything if detected nop
	if (inst->opcode == NOP_OP)
	{
		core->ex_mem.D_inst = inst;
		return;
	}
	
	if (VERBOSE_MODE) printf("executing alu op: %d\n",inst->opcode);
	// execute the ALU op
	switch(inst->opcode)
	{
	case(ADD_OP):
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(SUB_OP):
		core->ex_mem.D_alu_res = inst->rs - inst->rt;
		break;
	case(AND_OP):
		core->ex_mem.D_alu_res = inst->rs & inst->rt;
		break;
	case(OR_OP):
		core->ex_mem.D_alu_res = inst->rs | inst->rt;
		break;
	case(XOR_OP):
		core->ex_mem.D_alu_res = inst->rs ^ inst->rt;
		break;
	case(MUL_OP):
		core->ex_mem.D_alu_res = inst->rs * inst->rt;
		break;
	case(SLL_OP):
		core->ex_mem.D_alu_res = (inst->rs) << inst->rt;
		break;
	case(SRA_OP):
		core->ex_mem.D_alu_res = (inst->rs) >> inst->rt;
		break;
	case(SRL_OP):
		core->ex_mem.D_alu_res = ((inst->rs) >> inst->rt) & ~(1<<31);
		break;
	case(LW_OP):
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(SW_OP):
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(LL_OP):
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	case(SC_OP):
		core->ex_mem.D_alu_res = inst->rs + inst->rt;
		break;
	}
	if (VERBOSE_MODE) printf("res is: %d\n",core->ex_mem.D_alu_res);
	// update the next step in pipe
	core->memory_pc_D = core->execute_pc_Q;
	core->ex_mem.D_inst = inst;
	core->next_cycle_memory = Active;
	if (VERBOSE_MODE) printf("---- exit execute----\n");
}
void memory(Core* core,Cache* cache, struct WatchFlag** watch)
{
	if (VERBOSE_MODE) printf("---- enter memory---\n");
	if (core->memory == Stall)
	{
		if (VERBOSE_MODE) printf("not active\n");
		return;
	}
	bool is_stall = false;
	Instruction* inst = core->ex_mem.Q_inst;
	// dont do anything if detected nop
	if (inst->opcode == NOP_OP)
	{
			core->mem_wb.D_inst = inst;
			return;
	}
	int alu_res = core->ex_mem.Q_alu_res;

	// if it's a memory op (get/set), make the relevant request
	if(inst->opcode == SW_OP)
	{
		if (VERBOSE_MODE) printf("performing sw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, alu_res, false, false);
	}
	else if(inst->opcode == LW_OP)
	{
		if (VERBOSE_MODE) printf("performing lw\n");
		is_stall = !get_set_data_from_memory(core, inst, cache, alu_res, true, false);
	}
	else if(inst->opcode == LL_OP)
	{
		if (VERBOSE_MODE) printf("performing load linked\n");
		watch[(core->core_id)]->address  = alu_res;
		watch[(core->core_id)]->watching = true;
		watch[(core->core_id)]->visited  = false;
		is_stall = !get_set_data_from_memory(core, inst, cache, alu_res, true, false);
	}
	else if(inst->opcode == SC_OP)
	{
		if (VERBOSE_MODE) printf("performing store conditional\n");
		struct WatchFlag* cur_watch = watch[(core->core_id)];
		if(cur_watch->address == alu_res && cur_watch->watching && cur_watch->visited)
		{
			/*sc fail*/
			if (VERBOSE_MODE) printf("sc Failed\n");
			inst->rd = 0;
		}
		else
		{
			/*sc successes*/
			if (VERBOSE_MODE) printf("sc successes writing to memroy\n");
			is_stall = !get_set_data_from_memory(core, inst, cache, alu_res, false, true);
		}
		for(int i=0;i<NUM_CORES;i++)
		{
			if(i == core->core_id) continue;
			if(watch[i]->address == alu_res && watch[i]->watching)
			{
				watch[i]->visited = true;
				if (VERBOSE_MODE) printf("sc on other core ll\n");
			}
		}


	}
	/*arithmetic/logic op*/
	else if(inst->opcode <= SRL_OP)
	{
		if (VERBOSE_MODE) printf("set rd for write back\n");
		inst->rd = alu_res;
	}
	
	if (!is_stall)
	{
		// no memory stall, set the next step in pipe
		core->mem_wb.D_inst = inst;
		core->writeback_pc_D = core->memory_pc_Q;
	}
	else
	{
		handle_memory_hazard(core);
	}
	core->next_cycle_writeback = Active;

	if (VERBOSE_MODE) printf("----exit memory----\n");
}
bool write_back(Core* core){
	if (VERBOSE_MODE) printf("----enter writeback----\n");

	if (core->writeback == Stall)
	{
		if (VERBOSE_MODE) printf("not active\n");
		/*signal that core itself is still active*/
		return true;
	}

	Instruction* inst = core->mem_wb.Q_inst;
	if (inst->opcode == NOP_OP){
		// dont do anything if detected nop op
		free(inst);
		return true;
	}
	/*perform writeback*/
	if (inst->opcode<=SRL_OP/*arit/logi*/ || inst->opcode == LW_OP || inst->opcode == LL_OP){
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index)
		{
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write_D[inst->rd_index] = false; /*clean*/
	}
	else if(inst->opcode == JAL_OP)
	{
		if (VERBOSE_MODE) printf("writing back jal value %x to reg 15\n",inst->rd);
		core->regs[JAL_REG_IDX] = core->pc-4;
		core->regs_to_write_D[JAL_REG_IDX] = false;
	}
	if(inst->opcode == SC_OP)
	{
		if (VERBOSE_MODE) printf("writing back value %x to reg number %d\n",inst->rd,inst->rd_index);
		if(inst->rd_index)
		{
			core->regs[inst->rd_index] = inst->rd;
		}
		core->regs_to_write_D[inst->rd_index] = false; /*clean*/
	}
	if(inst->opcode == HALT_OP)
	{
		if (VERBOSE_MODE) printf("Core Halting\n");
		core->core_state_D = Halt;
		return false;
	}
	free(inst);
	// if something stalled update the pcs so that if the cmd from one stage to the pipe is trainstioned to the next, then the pc will be -1 ('---' in trace)
	if (core->memory_pc_Q == core->writeback_pc_Q || core->memory_pc_D == core->writeback_pc_D)
	{
		core->memory_pc_D = NOT_INITIALIZED;
	}
	if (core->execute_pc_Q == core->writeback_pc_Q || core->execute_pc_D == core->writeback_pc_D)
	{
		core->execute_pc_D = NOT_INITIALIZED;
	}
	if (core->decode_pc_Q == core->writeback_pc_Q || core->decode_pc_D == core->writeback_pc_D)
	{
		core->decode_pc_D = NOT_INITIALIZED;
	}
	if (core->writeback_pc_D == core->writeback_pc_Q)
	
	{
		core->writeback_pc_D = NOT_INITIALIZED;
	}
	if (VERBOSE_MODE) printf("----exit writeback----\n");
	return true;
}



