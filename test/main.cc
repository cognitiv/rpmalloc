#include <rpmalloc.h>
#include <vector>
#include <cstdio>

int main(int argc, char** argv) {

#ifdef RPMALLOC_FIRST_CLASS_HEAPS
	rpmalloc_initialize();

	auto heap = rpmalloc_heap_acquire();
	rp_heap_stl_allocator<int> alloc(heap);
	std::vector<int, rp_heap_stl_allocator<int>> vec(alloc);

	vec.resize(1024);
	std::vector<int, rp_heap_stl_allocator<int>> copy = vec;
	printf("%lu vs %lu\n", vec.size(), copy.size());

	rpmalloc_heap_free_all(heap);
	rpmalloc_heap_release(heap);
	
#endif
	return 0;
}
