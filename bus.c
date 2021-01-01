#include "bus.h"

unsigned short is_bus_free(Bus* bus) {
	return bus->bus_cmd == 0;
}

unsigned short is_data_ready_from_bus(Bus* bus, uint32_t addr)
{
	return bus->bus_addr == addr && bus->bus_cmd == 3;
}

int32_t get_data_from_bus(Bus* bus) {
	return bus->bus_data;
}

void make_BusRd_request(Bus* bus, int core_idx, uint32_t addr)
{
	bus->bus_origid = core_idx;
	bus->bus_cmd = 1;
	bus->bus_addr = addr;

}

void free_bus_line(Bus* bus) {
	bus->bus_cmd = 0;
}