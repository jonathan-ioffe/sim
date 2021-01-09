#ifndef BUS_H_
#define BUS_H_
#include "main.h"

typedef struct bus {
	unsigned short bus_origid_Q, bus_origid_D;
	enum BusCmd bus_cmd_Q, bus_cmd_D;
	uint32_t bus_addr_Q, bus_addr_D;
	int32_t bus_data_Q, bus_data_D;
}Bus;

Bus bus;

enum BusCmd {NoCommand, BudRd, BusRdX, Flush};

void init_bus(char* bus_trace_file_name);
void run_bus_cycle();
void bus_next_cycle();
bool is_bus_pending_flush(Bus* bus);
bool is_bus_pending_data(Bus* bus);
bool is_bus_free(Bus* bus);
bool is_data_ready_from_bus(Bus* bus, int core_idx, uint32_t addr);
int32_t get_data_from_bus(Bus* bus);
void make_BusRd_request(Bus* bus, int core_idx, uint32_t addr);
void make_BusRdX_request(Bus* bus, int core_idx, uint32_t addr);
void make_Flush_request(Bus* bus, int core_idx, uint32_t addr, int32_t data);
void free_bus_line(Bus* bus);
#endif /* BUS_H_ */
