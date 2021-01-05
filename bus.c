#include "bus.h"

char* bus_trace_fn;

void init_bus(char* bus_trace_file_name)
{
	if (VERBOSE_MODE) printf("Start init bus\n");
	bus.bus_cmd_Q = 0;
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
	int write_trace_line = bus.bus_cmd_D != 0 && bus.bus_cmd_D != bus.bus_cmd_Q;
	bus.bus_addr_Q = bus.bus_addr_D;
	bus.bus_cmd_Q = bus.bus_cmd_D;
	bus.bus_data_Q = bus.bus_data_D;
	bus.bus_origid_Q = bus.bus_origid_D;
	if (write_trace_line) write_bus_trace_line();
}

bool is_bus_free(Bus* bus) {
	return bus->bus_cmd_Q == 0;
}

bool is_data_ready_from_bus(Bus* bus, int core_idx, uint32_t addr)
{
	return bus->bus_addr_Q == addr && bus->bus_cmd_Q == 3;
}

bool is_bus_pending_flush(Bus* bus)
{
	return bus->bus_cmd_Q == 3;
}

bool is_bus_pending_data(Bus* bus)
{
	return bus->bus_cmd_Q != 0 && bus->bus_cmd_Q != 3;
}

int32_t get_data_from_bus(Bus* bus) {
	return bus->bus_data_Q;
}

void make_BusRd_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = 1;
	bus->bus_addr_D = addr;
	bus->bus_data_D = 0;

}

void make_BusRdX_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = 2;
	bus->bus_addr_D = addr;
	bus->bus_data_D = 0;
}

void make_Flush_request(Bus* bus, int core_idx, uint32_t addr, int32_t data)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = 3;
	bus->bus_addr_D = addr;
	bus->bus_data_D = data;
}

void free_bus_line(Bus* bus) {
	bus->bus_cmd_D = 0;
}