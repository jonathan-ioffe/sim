#include "cores.h"
#include "instruction.h"
#include "bus.h"
#include "cache.h"
Core* cores[NUM_CORES];
IM* inst_mems[NUM_CORES];
Cache* caches[NUM_CORES];
struct WatchFlag* watch[NUM_CORES];

int cores_running = 4;
int main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;
char** core_trace_fns;
char** regout_fns;
char** dsram_fns;
char** tsram_fns;
char** stats_fns;

// Handle file writes related to cores
void write_core_trace_line()
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		if (cores[i]->core_state_Q == Halt) {
			continue;
		}
		FILE* curr_core_trace_fd;
		curr_core_trace_fd = fopen(core_trace_fns[i], "a");

		int curr_pcs[5] = {
			cores[i]->fetch_pc_Q,
			cores[i]->decode_pc_Q == cores[i]->execute_pc_Q ? -1 : cores[i]->decode_pc_Q,
			cores[i]->execute_pc_Q == cores[i]->memory_pc_Q ? -1 : cores[i]->execute_pc_Q,
			cores[i]->memory_pc_Q == cores[i]->writeback_pc_Q ? -1 : cores[i]->memory_pc_Q,
			cores[i]->writeback_pc_Q
		};

		int write_line = 0;
		for (int j = 0; j < 5; j++)
		{
			if (curr_pcs[j] != -1)
			{
				write_line = 1;
				break;
			}
		}

		if (write_line)
		{
			fprintf(curr_core_trace_fd, "%d ", clk_cycles);
			for (int j = 0; j < 5; j++)
			{
				if (curr_pcs[j] == -1 || curr_pcs[j] >= cores[i]->halt_pc)
				{
					fprintf(curr_core_trace_fd, "--- ");
				}
				else
				{
					fprintf(curr_core_trace_fd, "%03X ", curr_pcs[j]);
				}
			}

			for (int j = 2; j < NUM_REGS; j++)
			{
				fprintf(curr_core_trace_fd, "%08X ", cores[i]->regs[j]);
			}
			fprintf(curr_core_trace_fd, "\n");
		}
		fclose(curr_core_trace_fd);
	}
}

void write_core_regs_files()
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_regout_fd;

		curr_regout_fd = fopen(regout_fns[i], "w");

		for (int j = 2; j < NUM_REGS; j++)
		{
			fprintf(curr_regout_fd, "%08X\n", cores[i]->regs[j]);
		}
		fclose(curr_regout_fd);
	}
}

void write_core_dsram_files()
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_dsram_fd;

		curr_dsram_fd = fopen(dsram_fns[i], "w");

		for (int j = 0; j < CACHE_SIZE; j++)
		{
			fprintf(curr_dsram_fd, "%08X\n", caches[i]->cache[j].data);
		}
		fclose(curr_dsram_fd);
	}
}

void write_core_tsram_files()
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_tsram_fd;

		curr_tsram_fd = fopen(tsram_fns[i], "w");

		for (int j = 0; j < CACHE_SIZE; j++)
		{
			fprintf(curr_tsram_fd, "%08X\n", (caches[i]->cache[j].state << 12) + caches[i]->cache[j].tag);
		}
		fclose(curr_tsram_fd);
	}
}

void write_core_stats_files()
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_stats_fd;

		curr_stats_fd = fopen(stats_fns[i], "w");

		fprintf(curr_stats_fd, "cycles %d\n", cores[i]->core_stats_counts.cycles);
		fprintf(curr_stats_fd, "instructions %d\n", cores[i]->core_stats_counts.instructions);
		fprintf(curr_stats_fd, "read_hit %d\n", cores[i]->core_stats_counts.read_hit);
		fprintf(curr_stats_fd, "write_hit %d\n", cores[i]->core_stats_counts.write_hit);
		fprintf(curr_stats_fd, "read_miss %d\n", cores[i]->core_stats_counts.read_miss);
		fprintf(curr_stats_fd, "write_miss %d\n", cores[i]->core_stats_counts.write_miss);
		fprintf(curr_stats_fd, "decode_stall %d\n", cores[i]->core_stats_counts.decode_stall);
		fprintf(curr_stats_fd, "mem_stall %d\n", cores[i]->core_stats_counts.mem_stall);

		fclose(curr_stats_fd);
	}
}

void write_cores_output_files()
{
	write_core_regs_files();
	write_core_dsram_files();
	write_core_tsram_files();
	write_core_stats_files();
}

// Handle stalls

// make the stall when encountered in decode stage (data hazard)
void handle_data_hazard(Core* core)
{
	if (VERBOSE_MODE) printf("hazard exists\n");
	if (!core->is_data_stall) core->core_stats_counts.decode_stall++; // count decode stall only if it's not stalling because of memory hazard before
	// stall the current step
	core->pc = core->pc_prev;
	core->if_id.D_inst = core->if_id.Q_inst;
	core->decode_pc_D = core->decode_pc_Q;

	// stall the next step
	Instruction* nop_inst = (Instruction*)calloc(1, sizeof(Instruction));
	nop_inst->opcode = NOP_OP;
	core->id_ex.D_inst = nop_inst;
	core->execute_pc_D = core->execute_pc_Q;

	// report data hazard
	core->is_data_hazard = true;
}

// make the stall when encountered in memory stage (waiting for data to arrive)
void handle_memory_hazard(Core* core)
{
	// there's memory stall, report it
	core->is_data_stall = true;
	core->core_stats_counts.mem_stall++;
	// stal previous steps in pipe
	core->pc = core->pc_prev;
	core->if_id.D_inst = core->if_id.Q_inst;
	core->id_ex.D_inst = core->id_ex.Q_inst;
	core->ex_mem.D_inst = core->ex_mem.Q_inst;
	core->ex_mem.D_alu_res = core->ex_mem.Q_alu_res;
	core->execute_pc_D = core->execute_pc_Q;
	core->decode_pc_D = core->decode_pc_Q;
	// stall current & next steps in pipe
	core->mem_wb.D_inst = (Instruction*)calloc(1, sizeof(Instruction));
	core->mem_wb.D_inst->opcode = NOP_OP;
	core->next_cycle_fetch = core->fetch;
	core->next_cycle_decode = core->decode;
	core->next_cycle_execute = core->execute;
	core->next_cycle_memory = core->memory;
	core->next_cycle_writeback = core->writeback;
	core->writeback_pc_D = core->writeback_pc_Q;
	core->memory_pc_D = core->memory_pc_Q;

	// stall the regs hazards FFs
	for (int i = 0; i < NUM_REGS; i++)
	{
		core->regs_to_write_D[i] = core->regs_to_write_Q[i];
	}
}

void init_pipe(int core_num){
	cores[core_num]->fetch = Active;
	cores[core_num]->decode = Stall;
	cores[core_num]->execute = Stall;
	cores[core_num]->memory = Stall;
	cores[core_num]->writeback = Stall;
	cores[core_num]->next_cycle_fetch = Active;
	cores[core_num]->next_cycle_decode = Stall;
	cores[core_num]->next_cycle_execute = Stall;
	cores[core_num]->next_cycle_memory = Stall;
	cores[core_num]->next_cycle_writeback = Stall;
}

void init_cores(char** core_trace_file_names, char** regout_file_names, char** dsram_file_names, char** tsram_file_names, char** stats_file_names)
{
    if (VERBOSE_MODE) printf("Start init cores\n");
    for(int i=0;i<NUM_CORES;i++)
	{
        cores[i] = (Core*)calloc(1,sizeof(Core));
        cores[i]->core_state_Q = Active; /*activate cores*/
		cores[i]->core_state_D = Active; /*activate cores*/
		cores[i]->pending_bus_read = Free;
        cores[i]->core_id = i;
        init_pipe(i);
        inst_mems[i] = (IM*)calloc(1,sizeof(IM));
        caches[i] = (Cache*)calloc(1,sizeof(Cache));
        watch[i] = (struct WatchFlag*)calloc(1,sizeof(struct WatchFlag));
		cores[i]->fetch_pc_Q = 0;
		cores[i]->decode_pc_Q = NOT_INITIALIZED;
		cores[i]->execute_pc_Q = NOT_INITIALIZED;
		cores[i]->memory_pc_Q = NOT_INITIALIZED;
		cores[i]->writeback_pc_Q = NOT_INITIALIZED;
		cores[i]->fetch_pc_D = 0;
		cores[i]->decode_pc_D = NOT_INITIALIZED;
		cores[i]->execute_pc_D = NOT_INITIALIZED;
		cores[i]->memory_pc_D = NOT_INITIALIZED;
		cores[i]->writeback_pc_D = NOT_INITIALIZED;
		cores[i]->halt_pc = NOT_INITIALIZED;
		cores[i]->core_stats_counts.cycles = NOT_INITIALIZED;
		cores[i]->if_id.Q_inst = NOT_INITIALIZED;
		cores[i]->id_ex.Q_inst = NOT_INITIALIZED;
		cores[i]->ex_mem.Q_inst = NOT_INITIALIZED;
		cores[i]->mem_wb.Q_inst = NOT_INITIALIZED;
		FILE* curr_core_trace_fd;
		curr_core_trace_fd = fopen(core_trace_file_names[i], "w");
		fclose(curr_core_trace_fd);
    }

	core_trace_fns = core_trace_file_names;
	regout_fns = regout_file_names;
	dsram_fns = dsram_file_names;
	tsram_fns = tsram_file_names;
	stats_fns = stats_file_names;

	if (VERBOSE_MODE) printf("Init cores done\n");
}

void load_inst_mems(char** inst_mems_file_names){
	char line[HEX_INST_LEN] = {0};
	for(int i=0;i<NUM_CORES;i++){
		FILE* fd = fopen(inst_mems_file_names[i],"r");
		int line_num = 0;
		while (fscanf(fd, "%s\n", line) != EOF) {
			inst_mems[i]->mem[line_num] = strtol(line, NULL, 16);
			line_num++;
		}
		fclose(fd);
	}
}

void handle_core_to_bus_requests(Core* core)
{
	if (core->pending_bus_read != Free)
	{
		if (is_bus_free(&bus))
		{
			if (core->pending_bus_read == MadeRd)
			{
				make_BusRd_request(&bus, core->core_id, core->pending_bus_read_addr);
			}
			else if (core->pending_bus_read == MadeRdX)
			{
				make_BusRdX_request(&bus, core->core_id, core->pending_bus_read_addr);
			}
			else if (core->pending_bus_read == MadeFlush)
			{
				make_Flush_request(&bus, core->core_id, core->pending_bus_read_addr, core->pending_bus_read_data);
			}
			core->pending_bus_read = Pending;
		}
		if (is_bus_pending_flush(&bus))
		{
			if (core->core_id == bus.bus_origid_Q)
			{
				core->pending_bus_read = Free;
			}
			else if (core->pending_bus_read == Pending && core->pending_bus_read_addr == bus.bus_addr_Q)
			{
				core->pending_bus_read = Ready;
				core->pending_bus_read_data = bus.bus_data_Q;
			}
		}
	}
}

void cores_next_cycle()
{
	for(int i=0;i<NUM_CORES;i++){
		cores[i]->if_id.Q_inst = cores[i]->if_id.D_inst;
		cores[i]->id_ex.Q_inst = cores[i]->id_ex.D_inst;
		cores[i]->ex_mem.Q_inst = cores[i]->ex_mem.D_inst;
		cores[i]->ex_mem.Q_alu_res = cores[i]->ex_mem.D_alu_res;
		cores[i]->mem_wb.Q_inst = cores[i]->mem_wb.D_inst;

		cores[i]->fetch = cores[i]->next_cycle_fetch;
		cores[i]->decode = cores[i]->next_cycle_decode;
		cores[i]->execute = cores[i]->next_cycle_execute;
		cores[i]->memory = cores[i]->next_cycle_memory;
		cores[i]->writeback = cores[i]->next_cycle_writeback;

		if (i == 2 && clk_cycles == 2)
		{
			printf("");
		}

		cores[i]->fetch_pc_D = cores[i]->pc;

		cores[i]->fetch_pc_Q = cores[i]->fetch_pc_D;
		cores[i]->decode_pc_Q = cores[i]->decode_pc_D;
		cores[i]->execute_pc_Q = cores[i]->execute_pc_D;
		cores[i]->memory_pc_Q = cores[i]->memory_pc_D;
		cores[i]->writeback_pc_Q = cores[i]->writeback_pc_D;
		cores[i]->core_state_Q = cores[i]->core_state_D;

		/*cores[i]->pending_bus_read_Q = cores[i]->pending_bus_read_D;
		cores[i]->pending_bus_read_addr_Q = cores[i]->pending_bus_read_addr_D;
		cores[i]->pending_bus_read_data_Q = cores[i]->pending_bus_read_data_D;	*/

		for (int j = 0; j < NUM_REGS; j++)
		{
			cores[i]->regs_to_write_Q[j] = cores[i]->regs_to_write_D[j];
		}
	}	
	
}

void run_program(uint32_t* MM) {
	if (VERBOSE_MODE) printf("Start running program\n");
	while (cores_running > 0)
	{
		if (is_bus_pending_flush(&bus))
		{
			MM[bus.bus_addr_Q] = bus.bus_data_Q;
			//if (bus.bus_origid_Q < NUM_CORES)
			//{
			//	//caches[bus.bus_origid_Q]->cache->state = bus.bus_cmd_Q == 1 ? S : I;
			//	cores[bus.bus_origid_Q]->pending_bus_read = Free;
			//}
			free_bus_line(&bus);
		}
		else if (is_bus_pending_data(&bus))
		{
			/* not fetching from main memory yet, check if in one of the caches */
			if (VERBOSE_MODE) printf("main stalls left: %d\n", main_memory_stalls_left);
			if (main_memory_stalls_left == MAIN_MEMORY_FETCH_DELAY)
			{
				bool found_in_core = false;
				for (int i = 0; i < NUM_CORES; i++)
				{
					if (bus.bus_origid_Q == i)
					{
						continue;
					}
					uint32_t addr = bus.bus_addr_Q;
					if (is_data_in_cache_dirty(caches[i], addr))
					{
						int32_t data = get_data_from_cache(caches[i], addr);
						make_Flush_request(&bus, i, addr, data);
						if (bus.bus_cmd_Q == 2) /*busrdx*/
						{
							caches[i]->cache[get_cache_index(bus.bus_addr_Q)].state = I;
						}
						else /* busrd */
						{
							caches[i]->cache[get_cache_index(bus.bus_addr_Q)].state = S;
						}
						found_in_core = true;
						break;
					}
					else if (is_data_in_cache(caches[i], addr) && bus.bus_cmd_Q == 2)
					{
						caches[i]->cache[get_cache_index(bus.bus_addr_Q)].state = I;
					}
				}
				/* invoke read from main memory next cycle*/
				if (!found_in_core)
				{
					main_memory_stalls_left--;
				}
			}
			/* fetched from main memory, flush to bus */
			/* count one less since the flush will happen only on next cycle */
			else if (main_memory_stalls_left == 1)
			{
				uint32_t addr = bus.bus_addr_Q;
				make_Flush_request(&bus, 4, addr, MM[addr]);
				/* reset the main stall counter for the next miss */
				main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;
			}
			/* pending main memory */
			else
			{
				main_memory_stalls_left--;
			}
		}
		write_core_trace_line();
		for(int i =0;i < NUM_CORES;i++)
		{
			if (cores[i]->core_state_Q == Halt)
			{
				continue;
			}
			if (VERBOSE_MODE) printf("$$$ core %d $$$\n",i);
			handle_core_to_bus_requests(cores[i]);
			//printf("pipe states: %d, %d, %d, %d, %d\n",cores[i]->fetch,cores[i]->decode,cores[i]->execute,cores[i]->memory,cores[i]->writeback);
			fetch(cores[i],inst_mems[i]);
			decode(cores[i]);
			execute(cores[i]);
			memory(cores[i],caches[i], watch);
			if(!writeBack(cores[i]))
			{
				cores_running--;
				cores[i]->core_stats_counts.cycles = clk_cycles+1;
			}
		}
		
	
		
		cores_next_cycle();
		clk_cycles++;
		bus_next_cycle();

		if (VERBOSE_MODE) printf("increment cycle %d\n", clk_cycles);
	}
}


