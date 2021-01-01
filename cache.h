#ifndef CACHE_H_
#define CACHE_H_
#include "cores.h"
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;


unsigned short is_data_in_cache(Cache* cache, uint32_t addr);
int32_t get_data_from_cache(Cache* cache, uint32_t addr);
void set_data_to_cache(Cache* cache, uint32_t addr, int32_t data, unsigned short is_dirty);
#endif /* CACHE_H_ */
