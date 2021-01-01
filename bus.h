#ifndef BUS_H_
#define BUS_H_

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

typedef struct bus {
	unsigned short bus_origid;
	unsigned short bus_cmd;
	uint32_t bus_addr;
	int32_t bus_data;
}Bus;

unsigned short is_bus_free(Bus* bus);
unsigned short is_data_ready_from_bus(Bus* bus, uint32_t addr);
int32_t get_data_from_bus(Bus* bus);
void make_BusRd_request(Bus* bus, int core_idx, uint32_t addr);
void free_bus_line(Bus* bus);

#endif /* BUS_H_ */
