#include "cores.h"
#include "instruction.h"
#include "bus.h"
#include "cache.h"
Core* cores[NUM_CORES];
IM* inst_mems[NUM_CORES];
Cache* caches[NUM_CORES];
struct WatchFlag* watch[NUM_CORES];
Bus bus;
int cores_running = 4;
int clk_cycles = 0;
int main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;

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

void init_cores(){
    printf("Start init cores\n");
    for(int i=0;i<NUM_CORES;i++){
        cores[i] = (Core*)calloc(1,sizeof(Core));
        cores[i]-> core_state = Active; /*activate cores*/
        cores[i]->core_id = i ;
        init_pipe(i);
        inst_mems[i] = (IM*)calloc(1,sizeof(IM));
        caches[i] = (Cache*)calloc(1,sizeof(Cache));
        watch[i] = (struct WatchFlag*)calloc(1,sizeof(struct WatchFlag));
    }
	bus.bus_cmd_Q = 0;
    printf("Init cores done\n");
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
void next_cycle(){
	printf("increment cycle\n");
	clk_cycles++;
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
	}
	bus.bus_addr_Q = bus.bus_addr_D;
	bus.bus_cmd_Q = bus.bus_cmd_D;
	bus.bus_data_Q = bus.bus_data_D;
	bus.bus_origid_Q = bus.bus_origid_D;
}

void run_program(uint32_t* MM){
	printf("Start running program\n");
	/* TODO need to check bus panding here - any core that has should put on bus if not need main memory to return after 64 cycles */
	int first_iter = 1;
	while(cores_running > 0){
		if(first_iter==1){
			first_iter = 0;
		}
		else{
			next_cycle();
		}
		if (is_bus_pending_flush(&bus))
		{
			MM[bus.bus_addr_Q] = bus.bus_data_Q;
			if (bus.bus_origid_Q < NUM_CORES)
			{
				caches[bus.bus_origid_Q]->cache->state = bus.bus_cmd_Q == 1 ? S : I;
			}
			free_bus_line(&bus);
		}
		for(int i =0;i < NUM_CORES;i++){
			if (cores[i]->core_state == Halt){
				continue;
			}
			if (is_bus_pending_data(&bus))
			{
				/* search for data in core */
				uint32_t addr = bus.bus_addr_Q;
				if (is_data_in_cache_dirty(caches[i], addr))
				{
					int32_t data = get_data_from_cache(caches[i], addr);
					make_Flush_request(&bus, i, addr, data);
				}
				/* didn't find data in any core, fetch from main */
				else if (i == NUM_CORES - 1)
				{
					if (main_memory_stalls_left > 0)
					{
						main_memory_stalls_left--;
					}
					else {
						make_Flush_request(&bus, 4, addr, MM[addr]);
						/* reset the main stall counter for the next miss */
						main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;
					}
				}
			}
			printf("$$$ core %d $$$\n",i);
			//printf("pipe states: %d, %d, %d, %d, %d\n",cores[i]->fetch,cores[i]->decode,cores[i]->execute,cores[i]->memory,cores[i]->writeback);
			fetch(cores[i],inst_mems[i]);
			decode(cores[i]);
			execute(cores[i]);
			memory(cores[i],caches[i], &bus, watch);
			if(writeBack(cores[i])==Halt){
				cores_running--;
			}
		}
	}
}

void sanity(uint32_t* MM){ /*for debug*/
	/*for(int i=0;i<NUM_CORES;i++){
		printf("REGFILE %d\n",i);
		for(int j=2;j<NUM_REGS;j++){
			printf("%x  ",cores[i]->regs[j]);
		}
		printf("\n");

	}*/
	printf("%x\n",cores[0]->regs[4]);
}




