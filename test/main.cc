#define RPMALLOC_FIRST_CLASS_HEAPS 1
#include "../rpmalloc/rpmalloc.h"
#include <vector>
#include <cstdio>

int main(int argc, char** argv) {

#ifdef RPMALLOC_FIRST_CLASS_HEAPS
	rpmalloc_initialize();

	rpmalloc_managed_heap heap(std::in_place_t{});
	rp_heap_stl_allocator<int> alloc(heap);
	std::vector<int, rp_heap_stl_allocator<int>> vec(alloc);

	for (int i = 0; i < 245760; ++i) {
		vec.push_back(i);
	}
	
	rpmalloc_managed_heap other_heap(std::in_place_t{});
	heap.set_heap_for_copy(other_heap);

	auto uptr = heap.make_unique<int>();
	auto sptr = heap.make_shared<int>();

	std::vector<int, rp_heap_stl_allocator<int>> copy = vec;
	printf("%lu vs %lu | heap_used: %lu vs %lu, heap total: %lu vs %lu\n",
			vec.size(), copy.size(),
			heap.get_used_size(), other_heap.get_used_size(),
			heap.get_total_size(), other_heap.get_total_size());
#endif
	return 0;
}
