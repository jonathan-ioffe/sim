#include "cache.h"

unsigned short get_cache_index(uint32_t addr) {
	return addr & 0xFF;
}

unsigned short get_cache_tag(uint32_t addr) {
	return addr >> 8;
}

uint32_t get_main_memory_addr(unsigned short index, unsigned short tag)
{
	return (tag << 8) + index;
}

unsigned short is_data_in_cache(Cache* cache, uint32_t addr)
{
	unsigned short index = get_cache_index(addr);
	unsigned short tag = get_cache_tag(addr);

	/* Check if the data is inavlid */
	if (cache->cache[index].state == I) {
		return 0;
	}
	/* Check if the tag is matching */
	if (cache->cache[index].tag != tag) {
		return 0;
	}

	return 1;
}

unsigned short is_data_in_cache_dirty(Cache* cache, uint32_t addr)
{
	unsigned short index = get_cache_index(addr);
	if (!is_data_in_cache(cache, addr)) {
		return 0;
	}
	/* let other cores read only if it's in M state */
	if (cache->cache[index].state == M) {
		return 1;
	}
	return 0;
}

int32_t get_data_from_cache(Cache* cache, uint32_t addr)
{
	return cache->cache[get_cache_index(addr)].data;
}

void set_data_to_cache(Cache* cache, uint32_t addr, int32_t data, unsigned short is_dirty)
{
	unsigned short index = get_cache_index(addr);
	cache->cache[index].tag = get_cache_tag(addr);
	cache->cache[index].state = is_dirty ? M : S;
	cache->cache[index].data = data;
}