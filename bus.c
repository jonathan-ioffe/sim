#include "bus.h"

unsigned short is_bus_free(Bus* bus) {
	return bus->bus_cmd_Q == 0;
}

unsigned short is_data_ready_from_bus(Bus* bus, int core_idx, uint32_t addr)
{
	return bus->bus_addr_Q == addr && bus->bus_cmd_Q == 3 && bus->bus_origid_Q != core_idx;
}

unsigned short is_bus_pending_flush(Bus* bus)
{
	return bus->bus_cmd_Q == 3;
}

unsigned short is_bus_pending_data(Bus* bus)
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

}

void make_BusRdX_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid_D = core_idx;
	bus->bus_cmd_D = 2;
	bus->bus_addr_D = addr;

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