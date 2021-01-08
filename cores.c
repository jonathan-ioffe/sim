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

void write_core_regs_files(char** regout_file_names)
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_regout_fd;

		curr_regout_fd = fopen(regout_file_names[i], "w");

		for (int j = 2; j < NUM_REGS; j++)
		{
			fprintf(curr_regout_fd, "%08X\n", cores[i]->regs[j]);
		}
		fclose(curr_regout_fd);
	}
}

void write_core_trace_line(char** core_trace_file_names)
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		if (cores[i]->core_state_Q == Halt) {
			continue;
		}
		FILE* curr_core_trace_fd;
		curr_core_trace_fd = fopen(core_trace_file_names[i], "a");

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

void write_core_dsram_files(char** dsram_file_names)
{
	for (int i = 0; i < NUM_CORES; i++)
	{
		FILE* curr_dsram_fd;

		curr_dsram_fd = fopen(dsram_file_names[i], "w");

		for (int j = 0; j < CACHE_SIZE; j++)
		{
			fprintf(curr_dsram_fd, "%08X\n", caches[i]->cache[j].data);
		}
		fclose(curr_dsram_fd);
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

void init_cores(char** core_trace_file_names){
    if (VERBOSE_MODE) printf("Start init cores\n");
    for(int i=0;i<NUM_CORES;i++){
        cores[i] = (Core*)calloc(1,sizeof(Core));
        cores[i]-> core_state_Q = Active; /*activate cores*/
		cores[i]->core_state_D = Active; /*activate cores*/
		cores[i]->pending_bus_read = Free;
        cores[i]->core_id = i;
        init_pipe(i);
        inst_mems[i] = (IM*)calloc(1,sizeof(IM));
        caches[i] = (Cache*)calloc(1,sizeof(Cache));
        watch[i] = (struct WatchFlag*)calloc(1,sizeof(struct WatchFlag));
		cores[i]->fetch_pc_Q = 0;
		cores[i]->decode_pc_Q = -1;
		cores[i]->execute_pc_Q = -1;
		cores[i]->memory_pc_Q = -1;
		cores[i]->writeback_pc_Q = -1;
		cores[i]->fetch_pc_D = 0;
		cores[i]->decode_pc_D = -1;
		cores[i]->execute_pc_D = -1;
		cores[i]->memory_pc_D = -1;
		cores[i]->writeback_pc_D = -1;
		cores[i]->halt_pc = -1;
		FILE* curr_core_trace_fd;
		curr_core_trace_fd = fopen(core_trace_file_names[i], "w");
		fclose(curr_core_trace_fd);
    }
	core_trace_fns = core_trace_file_names;
	if (VERBOSE_MODE) printf("Init cores done\n");
    return;
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
				int found_in_core = 0;
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
							caches[i]->cache->tag = I;
						}
						else /* busrd */
						{
							caches[i]->cache->tag = S;
						}
						found_in_core = 1;
						break;
					}
					else if (is_data_in_cache(caches[i], addr) && bus.bus_cmd_Q == 2)
					{
						caches[i]->cache->tag = I;
					}
				}
				/* invoke read from main memory next cycle*/
				if (found_in_core == 0)
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
		write_core_trace_line(core_trace_fns);
		for(int i =0;i < NUM_CORES;i++){
			if (cores[i]->core_state_Q == Halt){
				continue;
			}
			if (i == 1 && clk_cycles == 6)
			{
				printf("");
			}
			if (VERBOSE_MODE) printf("$$$ core %d $$$\n",i);
			handle_core_to_bus_requests(cores[i]);
			//printf("pipe states: %d, %d, %d, %d, %d\n",cores[i]->fetch,cores[i]->decode,cores[i]->execute,cores[i]->memory,cores[i]->writeback);
			fetch(cores[i],inst_mems[i]);
			decode(cores[i]);
			execute(cores[i]);
			memory(cores[i],caches[i], &bus, watch);
			if(writeBack(cores[i])==Halt){
				cores_running--;
			}
		}
		
	
		
		cores_next_cycle();
		clk_cycles++;
		bus_next_cycle();

		if (VERBOSE_MODE) printf("increment cycle %d\n", clk_cycles);
	}
}


