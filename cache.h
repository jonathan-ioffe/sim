#ifndef CACHE_H_
#define CACHE_H_
#include "main.h"
#include "cores.h"


unsigned short get_cache_index(uint32_t addr);
uint32_t get_main_memory_addr(unsigned short index, unsigned short tag);
unsigned short is_data_in_cache(Cache* cache, uint32_t addr);
unsigned short is_data_in_cache_dirty(Cache* cache, uint32_t addr);
int32_t get_data_from_cache(Cache* cache, uint32_t addr);
void set_data_to_cache(Cache* cache, uint32_t addr, int32_t data, unsigned short is_dirty);
#endif /* CACHE_H_ */
