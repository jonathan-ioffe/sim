#ifndef BUS_H_
#define BUS_H_

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;

typedef struct bus {
	unsigned short bus_origid;
	unsigned short bus_cmd;
	uint32_t bus_addr;
	uint32_t bus_data;
}Bus;

#endif /* BUS_H_ */
