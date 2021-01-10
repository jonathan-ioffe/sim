#include "bus.h"
#include "cache.h"
#include "main_memory.h"
char* bus_trace_fn;
bool write_trace_line;

void init_bus(char* bus_trace_file_name)
{
	if (VERBOSE_MODE) printf("Start init bus\n");
	// start bus as free
	bus.bus_cmd_Q = NoCommand;
	// create the trace file for the bus
	FILE* curr_bus_trace_fd;
	curr_bus_trace_fd = fopen(bus_trace_file_name, "w");
	fclose(curr_bus_trace_fd);
	bus_trace_fn = bus_trace_file_name;
	if (VERBOSE_MODE) printf("Init bus done\n");
}

void write_bus_trace_line()
{
	FILE* curr_bus_trace_fd;
	curr_bus_trace_fd = fopen(bus_trace_fn, "a");

	fprintf(curr_bus_trace_fd, "%d %01X %01X %05X %08X\n", clk_cycles, bus.bus_origid_Q, bus.bus_cmd_Q, bus.bus_addr_Q, bus.bus_data_Q);

	fclose(curr_bus_trace_fd);
}

void bus_next_cycle()
{
	// we want to write a trace line in the next cycle only if theres a change in the bus state, and it's not to be free
	write_trace_line = bus.bus_cmd_D != NoCommand && bus.bus_cmd_D != bus.bus_cmd_Q;
	bus.bus_addr_Q = bus.bus_addr_D;
	bus.bus_cmd_Q = bus.bus_cmd_D;
	bus.bus_data_Q = bus.bus_data_D;
	bus.bus_origid_Q = bus.bus_origid_D;
}

bool is_bus_free(Bus* bus) 
{
	return bus->bus_cmd_Q == NoCommand && bus->bus_cmd_D == NoCommand;
}

bool is_data_ready_from_bus(Bus* bus, int core_idx, uint32_t addr)
{
	return bus->bus_addr_Q == addr && bus->bus_cmd_Q == Flush;
}

bool is_bus_pending_flush(Bus* bus)
{
	return bus->bus_cmd_Q == Flush;
}

bool is_bus_pending_data(Bus* bus)
{
	return bus->bus_cmd_Q != NoCommand && bus->bus_cmd_Q != Flush;
}

int32_t get_data_from_bus(Bus* bus) {
	return bus->bus_data_Q;
}

void make_BusRd_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = BudRd;
	bus->bus_addr_D = addr;
	bus->bus_data_D = 0;

}

void make_BusRdX_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = BusRdX;
	bus->bus_addr_D = addr;
	bus->bus_data_D = 0;
}

void make_Flush_request(Bus* bus, int core_idx, uint32_t addr, int32_t data)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = Flush;
	bus->bus_addr_D = addr;
	bus->bus_data_D = data;
}

void free_bus_line(Bus* bus) 
{
	bus->bus_cmd_D = NoCommand;
}

void run_bus_cycle()
{
	if (write_trace_line) write_bus_trace_line();
	if (is_bus_pending_flush(&bus))
	{
		MainMemory[bus.bus_addr_Q] = bus.bus_data_Q;
		free_bus_line(&bus);
	}
	else if (is_bus_pending_data(&bus))
	{
		// not fetching from main memory yet, check if in one of the caches
		if (VERBOSE_MODE) printf("main stalls left: %d\n", main_memory_stalls_left);
		if (main_memory_stalls_left == MAIN_MEMORY_FETCH_DELAY)
		{
			bool found_in_core = false;
			for (int i = 0; i < NUM_CORES; i++)
			{
				// ignore ths core that originated the request
				if (bus.bus_origid_Q == i)
				{
					continue;
				}
				uint32_t addr = bus.bus_addr_Q;
				if (is_data_in_cache_dirty(caches[i], addr))
				{
					// the requested data is present in the cache of curr core in M mode, flush it and transition to S/I as needed
					int32_t data = get_data_from_cache(caches[i], addr);
					make_Flush_request(&bus, i, addr, data);
					if (bus.bus_cmd_Q == BusRdX)
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
				else if (is_data_in_cache(caches[i], addr) && bus.bus_cmd_Q == BusRdX)
				{
					// the requested data in present in the cache of curr core, but another core requested exclusive read, so invalidate the data
					caches[i]->cache[get_cache_index(bus.bus_addr_Q)].state = I;
				}
			}
			// invoke read from main memory next cycle
			if (!found_in_core)
			{
				main_memory_stalls_left--;
			}
		}
		/* 
		 * fetched from main memory, flush to bus.
		 * count one less since the flush will happen only on next cycle 
		 */
		else if (main_memory_stalls_left == 1)
		{
			uint32_t addr = bus.bus_addr_Q;
			make_Flush_request(&bus, NUM_CORES, addr, MainMemory[addr]);
			// reset the main stall counter for the next miss
			main_memory_stalls_left = MAIN_MEMORY_FETCH_DELAY;
		}
		// pending main memory
		else
		{
			main_memory_stalls_left--;
		}
	}
}