#ifndef CACHE_H_
#define CACHE_H_
#include "main.h"
#include "cores.h"

Cache* caches[NUM_CORES];
unsigned short get_cache_index(uint32_t addr);
uint32_t get_main_memory_addr(unsigned short index, unsigned short tag);
bool is_data_in_cache(Cache* cache, uint32_t addr);
bool is_data_in_cache_dirty(Cache* cache, uint32_t addr);
int32_t get_data_from_cache(Cache* cache, uint32_t addr);
void set_data_to_cache(Cache* cache, uint32_t addr, int32_t data, bool is_dirty);
void invalidate_cache_addr(Cache* cache, uint32_t addr);
#endif /* CACHE_H_ */
