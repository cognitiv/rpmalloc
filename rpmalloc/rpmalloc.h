/* rpmalloc.h  -  Memory allocator  -  Public Domain  -  2016 Mattias Jansson
 *
 * This library provides a cross-platform lock free thread caching malloc implementation in C11.
 * The latest source code is always available at
 *
 * https://github.com/mjansson/rpmalloc
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__clang__) || defined(__GNUC__)
# define RPMALLOC_EXPORT __attribute__((visibility("default")))
# define RPMALLOC_ALLOCATOR
# if (defined(__clang_major__) && (__clang_major__ < 4)) || (defined(__GNUC__) && defined(ENABLE_PRELOAD) && ENABLE_PRELOAD)
# define RPMALLOC_ATTRIB_MALLOC
# define RPMALLOC_ATTRIB_ALLOC_SIZE(size)
# define RPMALLOC_ATTRIB_ALLOC_SIZE2(count, size)
# else
# define RPMALLOC_ATTRIB_MALLOC __attribute__((__malloc__))
# define RPMALLOC_ATTRIB_ALLOC_SIZE(size) __attribute__((alloc_size(size)))
# define RPMALLOC_ATTRIB_ALLOC_SIZE2(count, size)  __attribute__((alloc_size(count, size)))
# endif
# define RPMALLOC_CDECL
#elif defined(_MSC_VER)
# define RPMALLOC_EXPORT
# define RPMALLOC_ALLOCATOR __declspec(allocator) __declspec(restrict)
# define RPMALLOC_ATTRIB_MALLOC
# define RPMALLOC_ATTRIB_ALLOC_SIZE(size)
# define RPMALLOC_ATTRIB_ALLOC_SIZE2(count,size)
# define RPMALLOC_CDECL __cdecl
#else
# define RPMALLOC_EXPORT
# define RPMALLOC_ALLOCATOR
# define RPMALLOC_ATTRIB_MALLOC
# define RPMALLOC_ATTRIB_ALLOC_SIZE(size)
# define RPMALLOC_ATTRIB_ALLOC_SIZE2(count,size)
# define RPMALLOC_CDECL
#endif

//! Define RPMALLOC_CONFIGURABLE to enable configuring sizes. Will introduce
//  a very small overhead due to some size calculations not being compile time constants
#ifndef RPMALLOC_CONFIGURABLE
#define RPMALLOC_CONFIGURABLE 0
#endif

//! Define RPMALLOC_FIRST_CLASS_HEAPS to enable heap based API (rpmalloc_heap_* functions).
//  Will introduce a very small overhead to track fully allocated spans in heaps
#ifndef RPMALLOC_FIRST_CLASS_HEAPS
#define RPMALLOC_FIRST_CLASS_HEAPS 0
#endif

//! Flag to rpaligned_realloc to not preserve content in reallocation
#define RPMALLOC_NO_PRESERVE    1
//! Flag to rpaligned_realloc to fail and return null pointer if grow cannot be done in-place,
//  in which case the original pointer is still valid (just like a call to realloc which failes to allocate
//  a new block).
#define RPMALLOC_GROW_OR_FAIL   2

typedef struct rpmalloc_global_statistics_t {
	//! Current amount of virtual memory mapped, all of which might not have been committed (only if ENABLE_STATISTICS=1)
	size_t mapped;
	//! Peak amount of virtual memory mapped, all of which might not have been committed (only if ENABLE_STATISTICS=1)
	size_t mapped_peak;
	//! Current amount of memory in global caches for small and medium sizes (<32KiB)
	size_t cached;
	//! Current amount of memory allocated in huge allocations, i.e larger than LARGE_SIZE_LIMIT which is 2MiB by default (only if ENABLE_STATISTICS=1)
	size_t huge_alloc;
	//! Peak amount of memory allocated in huge allocations, i.e larger than LARGE_SIZE_LIMIT which is 2MiB by default (only if ENABLE_STATISTICS=1)
	size_t huge_alloc_peak;
	//! Total amount of memory mapped since initialization (only if ENABLE_STATISTICS=1)
	size_t mapped_total;
	//! Total amount of memory unmapped since initialization  (only if ENABLE_STATISTICS=1)
	size_t unmapped_total;
} rpmalloc_global_statistics_t;

typedef struct rpmalloc_thread_statistics_t {
	//! Current number of bytes available in thread size class caches for small and medium sizes (<32KiB)
	size_t sizecache;
	//! Current number of bytes available in thread span caches for small and medium sizes (<32KiB)
	size_t spancache;
	//! Total number of bytes transitioned from thread cache to global cache (only if ENABLE_STATISTICS=1)
	size_t thread_to_global;
	//! Total number of bytes transitioned from global cache to thread cache (only if ENABLE_STATISTICS=1)
	size_t global_to_thread;
	//! Per span count statistics (only if ENABLE_STATISTICS=1)
	struct {
		//! Currently used number of spans
		size_t current;
		//! High water mark of spans used
		size_t peak;
		//! Number of spans transitioned to global cache
		size_t to_global;
		//! Number of spans transitioned from global cache
		size_t from_global;
		//! Number of spans transitioned to thread cache
		size_t to_cache;
		//! Number of spans transitioned from thread cache
		size_t from_cache;
		//! Number of spans transitioned to reserved state
		size_t to_reserved;
		//! Number of spans transitioned from reserved state
		size_t from_reserved;
		//! Number of raw memory map calls (not hitting the reserve spans but resulting in actual OS mmap calls)
		size_t map_calls;
	} span_use[64];
	//! Per size class statistics (only if ENABLE_STATISTICS=1)
	struct {
		//! Current number of allocations
		size_t alloc_current;
		//! Peak number of allocations
		size_t alloc_peak;
		//! Total number of allocations
		size_t alloc_total;
		//! Total number of frees
		size_t free_total;
		//! Number of spans transitioned to cache
		size_t spans_to_cache;
		//! Number of spans transitioned from cache
		size_t spans_from_cache;
		//! Number of spans transitioned from reserved state
		size_t spans_from_reserved;
		//! Number of raw memory map calls (not hitting the reserve spans but resulting in actual OS mmap calls)
		size_t map_calls;
	} size_use[128];
} rpmalloc_thread_statistics_t;

typedef struct rpmalloc_config_t {
	//! Map memory pages for the given number of bytes. The returned address MUST be
	//  aligned to the rpmalloc span size, which will always be a power of two.
	//  Optionally the function can store an alignment offset in the offset variable
	//  in case it performs alignment and the returned pointer is offset from the
	//  actual start of the memory region due to this alignment. The alignment offset
	//  will be passed to the memory unmap function. The alignment offset MUST NOT be
	//  larger than 65535 (storable in an uint16_t), if it is you must use natural
	//  alignment to shift it into 16 bits. If you set a memory_map function, you
	//  must also set a memory_unmap function or else the default implementation will
	//  be used for both. This function must be thread safe, it can be called by
	//  multiple threads simultaneously.
	void* (*memory_map)(size_t size, size_t* offset);
	//! Unmap the memory pages starting at address and spanning the given number of bytes.
	//  If release is set to non-zero, the unmap is for an entire span range as returned by
	//  a previous call to memory_map and that the entire range should be released. The
	//  release argument holds the size of the entire span range. If release is set to 0,
	//  the unmap is a partial decommit of a subset of the mapped memory range.
	//  If you set a memory_unmap function, you must also set a memory_map function or
	//  else the default implementation will be used for both. This function must be thread
	//  safe, it can be called by multiple threads simultaneously.
	void (*memory_unmap)(void* address, size_t size, size_t offset, size_t release);
	//! Called when an assert fails, if asserts are enabled. Will use the standard assert()
	//  if this is not set.
	void (*error_callback)(const char* message);
	//! Called when a call to map memory pages fails (out of memory). If this callback is
	//  not set or returns zero the library will return a null pointer in the allocation
	//  call. If this callback returns non-zero the map call will be retried. The argument
	//  passed is the number of bytes that was requested in the map call. Only used if
	//  the default system memory map function is used (memory_map callback is not set).
	int (*map_fail_callback)(size_t size);
	//! Size of memory pages. The page size MUST be a power of two. All memory mapping
	//  requests to memory_map will be made with size set to a multiple of the page size.
	//  Used if RPMALLOC_CONFIGURABLE is defined to 1, otherwise system page size is used.
	size_t page_size;
	//! Size of a span of memory blocks. MUST be a power of two, and in [4096,262144]
	//  range (unless 0 - set to 0 to use the default span size). Used if RPMALLOC_CONFIGURABLE
	//  is defined to 1.
	size_t span_size;
	//! Number of spans to map at each request to map new virtual memory blocks. This can
	//  be used to minimize the system call overhead at the cost of virtual memory address
	//  space. The extra mapped pages will not be written until actually used, so physical
	//  committed memory should not be affected in the default implementation. Will be
	//  aligned to a multiple of spans that match memory page size in case of huge pages.
	size_t span_map_count;
	//! Enable use of large/huge pages. If this flag is set to non-zero and page size is
	//  zero, the allocator will try to enable huge pages and auto detect the configuration.
	//  If this is set to non-zero and page_size is also non-zero, the allocator will
	//  assume huge pages have been configured and enabled prior to initializing the
	//  allocator.
	//  For Windows, see https://docs.microsoft.com/en-us/windows/desktop/memory/large-page-support
	//  For Linux, see https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt
	int enable_huge_pages;
	//! Respectively allocated pages and huge allocated pages names for systems
	//  supporting it to be able to distinguish among anonymous regions.
	const char *page_name;
	const char *huge_page_name;
} rpmalloc_config_t;

//! Initialize allocator with default configuration
RPMALLOC_EXPORT int
rpmalloc_initialize(void);

//! Initialize allocator with given configuration
RPMALLOC_EXPORT int
rpmalloc_initialize_config(const rpmalloc_config_t* config);

//! Get allocator configuration
RPMALLOC_EXPORT const rpmalloc_config_t*
rpmalloc_config(void);

//! Finalize allocator
RPMALLOC_EXPORT void
rpmalloc_finalize(void);

//! Initialize allocator for calling thread
RPMALLOC_EXPORT void
rpmalloc_thread_initialize(void);

//! Finalize allocator for calling thread
RPMALLOC_EXPORT void
rpmalloc_thread_finalize(int release_caches);

//! Perform deferred deallocations pending for the calling thread heap
RPMALLOC_EXPORT void
rpmalloc_thread_collect(void);

//! Query if allocator is initialized for calling thread
RPMALLOC_EXPORT int
rpmalloc_is_thread_initialized(void);

//! Get per-thread statistics
RPMALLOC_EXPORT void
rpmalloc_thread_statistics(rpmalloc_thread_statistics_t* stats);

//! Get global statistics
RPMALLOC_EXPORT void
rpmalloc_global_statistics(rpmalloc_global_statistics_t* stats);

//! Dump all statistics in human readable format to file (should be a FILE*)
RPMALLOC_EXPORT void
rpmalloc_dump_statistics(void* file);

//! Allocate a memory block of at least the given size
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc(size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(1);

//! Free the given memory block
RPMALLOC_EXPORT void
rpfree(void* ptr);

//! Allocate a memory block of at least the given size and zero initialize it
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpcalloc(size_t num, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE2(1, 2);

//! Reallocate the given block to at least the given size
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rprealloc(void* ptr, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(2);

//! Reallocate the given block to at least the given size and alignment,
//  with optional control flags (see RPMALLOC_NO_PRESERVE).
//  Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB)
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpaligned_realloc(void* ptr, size_t alignment, size_t size, size_t oldsize, unsigned int flags) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(3);

//! Allocate a memory block of at least the given size and alignment.
//  Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB)
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpaligned_alloc(size_t alignment, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(2);

//! Allocate a memory block of at least the given size and alignment, and zero initialize it.
//  Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB)
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpaligned_calloc(size_t alignment, size_t num, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE2(2, 3);

//! Allocate a memory block of at least the given size and alignment.
//  Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB)
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmemalign(size_t alignment, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(2);

//! Allocate a memory block of at least the given size and alignment.
//  Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB)
RPMALLOC_EXPORT int
rpposix_memalign(void** memptr, size_t alignment, size_t size);

//! Query the usable size of the given memory block (from given pointer to the end of block)
RPMALLOC_EXPORT size_t
rpmalloc_usable_size(void* ptr);

//! Dummy empty function for forcing linker symbol inclusion
RPMALLOC_EXPORT void
rpmalloc_linker_reference(void);

#if RPMALLOC_FIRST_CLASS_HEAPS

//! Heap type
typedef struct heap_t rpmalloc_heap_t;

//! Acquire a new heap. Will reuse existing released heaps or allocate memory for a new heap
//  if none available. Heap API is implemented with the strict assumption that only one single
//  thread will call heap functions for a given heap at any given time, no functions are thread safe.
RPMALLOC_EXPORT rpmalloc_heap_t*
rpmalloc_heap_acquire(void);

//! Release a heap (does NOT free the memory allocated by the heap, use rpmalloc_heap_free_all before destroying the heap).
//  Releasing a heap will enable it to be reused by other threads. Safe to pass a null pointer.
RPMALLOC_EXPORT void
rpmalloc_heap_release(rpmalloc_heap_t* heap);

//! Allocate a memory block of at least the given size using the given heap.
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_alloc(rpmalloc_heap_t* heap, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(2);

//! Allocate a memory block of at least the given size using the given heap. The returned
//  block will have the requested alignment. Alignment must be a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB).
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_alloc(rpmalloc_heap_t* heap, size_t alignment, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(3);

//! Allocate a memory block of at least the given size using the given heap and zero initialize it.
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_calloc(rpmalloc_heap_t* heap, size_t num, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE2(2, 3);

//! Allocate a memory block of at least the given size using the given heap and zero initialize it. The returned
//  block will have the requested alignment. Alignment must either be zero, or a power of two and a multiple of sizeof(void*),
//  and should ideally be less than memory page size. A caveat of rpmalloc
//  internals is that this must also be strictly less than the span size (default 64KiB).
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_calloc(rpmalloc_heap_t* heap, size_t alignment, size_t num, size_t size) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE2(2, 3);

//! Reallocate the given block to at least the given size. The memory block MUST be allocated
//  by the same heap given to this function.
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_realloc(rpmalloc_heap_t* heap, void* ptr, size_t size, unsigned int flags) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(3);

//! Reallocate the given block to at least the given size. The memory block MUST be allocated
//  by the same heap given to this function. The returned block will have the requested alignment.
//  Alignment must be either zero, or a power of two and a multiple of sizeof(void*), and should ideally be
//  less than memory page size. A caveat of rpmalloc internals is that this must also be strictly less than
//  the span size (default 64KiB).
RPMALLOC_EXPORT RPMALLOC_ALLOCATOR void*
rpmalloc_heap_aligned_realloc(rpmalloc_heap_t* heap, void* ptr, size_t alignment, size_t size, unsigned int flags) RPMALLOC_ATTRIB_MALLOC RPMALLOC_ATTRIB_ALLOC_SIZE(4);

//! Free the given memory block from the given heap. The memory block MUST be allocated
//  by the same heap given to this function.
RPMALLOC_EXPORT void
rpmalloc_heap_free(rpmalloc_heap_t* heap, void* ptr);

//! Free all memory allocated by the heap
RPMALLOC_EXPORT void
rpmalloc_heap_free_all(rpmalloc_heap_t* heap);

//! Set the given heap as the current heap for the calling thread. A heap MUST only be current heap
//  for a single thread, a heap can never be shared between multiple threads. The previous
//  current heap for the calling thread is released to be reused by other threads.
RPMALLOC_EXPORT void
rpmalloc_heap_thread_set_current(rpmalloc_heap_t* heap);

RPMALLOC_EXPORT size_t
rpmalloc_heap_get_used_size(rpmalloc_heap_t* heap);

RPMALLOC_EXPORT size_t
rpmalloc_heap_get_total_size(rpmalloc_heap_t* heap);

#endif

#ifdef __cplusplus
}

#include <cstddef>     // std::size_t
#include <cstdint>     // PTRDIFF_MAX
#if (__cplusplus >= 201103L) || (_MSC_VER > 1900)  // C++11
#include <type_traits> // std::true_type
#include <utility>     // std::forward
#endif

template<class T> struct _rp_stl_allocator_common {
  typedef T                 value_type;
  typedef std::size_t       size_type;
  typedef std::ptrdiff_t    difference_type;
  typedef value_type&       reference;
  typedef value_type const& const_reference;
  typedef value_type*       pointer;
  typedef value_type const* const_pointer;

  #if ((__cplusplus >= 201103L) || (_MSC_VER > 1900))  // C++11
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap            = std::true_type;
  template <class U, class ...Args> void construct(U* p, Args&& ...args) { ::new(p) U(std::forward<Args>(args)...); }
  template <class U> void destroy(U* p) noexcept { p->~U(); }
  #else
  void construct(pointer p, value_type const& val) { ::new(p) value_type(val); }
  void destroy(pointer p) { p->~value_type(); }
  #endif

  size_type     max_size() const noexcept { return (PTRDIFF_MAX/sizeof(value_type)); }
  pointer       address(reference x) const        { return &x; }
  const_pointer address(const_reference x) const  { return &x; }
};

template<class T> struct rp_stl_allocator : public _rp_stl_allocator_common<T> {
  using typename _rp_stl_allocator_common<T>::size_type;
  using typename _rp_stl_allocator_common<T>::value_type;
  using typename _rp_stl_allocator_common<T>::pointer;
  template <class U> struct rebind { typedef rp_stl_allocator<U> other; };

  rp_stl_allocator()                                             noexcept = default;
  rp_stl_allocator(const rp_stl_allocator&)                      noexcept = default;
  template<class U> rp_stl_allocator(const rp_stl_allocator<U>&) noexcept { }
  rp_stl_allocator  select_on_container_copy_construction() const { return *this; }
  void              deallocate(T* p, size_type) { rpfree(p); }

  #if (__cplusplus >= 201703L)  // C++17
  [[nodiscard]] T* allocate(size_type count) { return static_cast<T*>(rpcalloc(count, sizeof(T))); }
  [[nodiscard]] T* allocate(size_type count, const void*) { return allocate(count); }
  #else
  [[nodiscard]] pointer allocate(size_type count, const void* = 0) { return static_cast<pointer>(rpcalloc(count, sizeof(value_type))); }
  #endif

  #if ((__cplusplus >= 201103L) || (_MSC_VER > 1900))  // C++11
  using is_always_equal = std::true_type;
  #endif
};


template<class T1,class T2> bool operator==(const rp_stl_allocator<T1>& , const rp_stl_allocator<T2>& ) noexcept { return true; }
template<class T1,class T2> bool operator!=(const rp_stl_allocator<T1>& , const rp_stl_allocator<T2>& ) noexcept { return false; }

#if RPMALLOC_FIRST_CLASS_HEAPS

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1900)  // C++11
#define MI_HAS_HEAP_STL_ALLOCATOR 1

#include <memory>      // std::shared_pt
#include <utility>
#include <stdexcept>

template<typename T>
struct rpmalloc_deleter {
	void operator()(T* t) {
		(*t).~T();
		rpfree(t);
	}
};

template <typename T>
struct rpmalloc_ptr {
    T* value;

    rpmalloc_ptr() : value(nullptr) {}

    explicit rpmalloc_ptr(T* ptr) : value(ptr) {}

		template<typename U>
    rpmalloc_ptr(const rpmalloc_ptr<U>& other) : value(other.value) {}

    T& operator[](std::size_t i) const {
        return value[i];
    }

    T* operator->() const {
        return value;
    }

    typename std::add_lvalue_reference<T> operator*() const {
        return *value;
    }

    void reset() {
        value->~T();
        rpfree(value);
        value = nullptr;
    }
};

template<typename T>
using rpmalloc_unique_ptr = std::unique_ptr<T, rpmalloc_deleter<T>>;

/** Provides container for managing a first class rmpalloc heap.
 *
 * By default the unique heap is empty (nullptr), it must be initialized with std::in_place_t to
 * create a usable heap. By default on copy the source heap will be used for the dest heap. Users
 * may optionally declare a different heap to be used on copy using (set_heap_on_copy()).
 *
 * This data-structure is not thread-safe, just as the heap itself is not thread-safe.
 */
struct rpmalloc_managed_heap {
	rpmalloc_managed_heap() = default;

	explicit rpmalloc_managed_heap(std::in_place_t t)
		: storage_(new storage()) {}

	rpmalloc_managed_heap(const rpmalloc_managed_heap& other)
	{
		storage_ = other.use_current_heap_on_copy() ? other.storage_ : other.storage_->heap_for_copy;

		if (storage_) {
			storage_->increment();
		}
	};

	rpmalloc_managed_heap(rpmalloc_managed_heap&& other) = delete;

	~rpmalloc_managed_heap() {
		if (storage_) {
			storage_->decrement();
		}
	}

	rpmalloc_managed_heap& operator=(rpmalloc_managed_heap&& other) = delete;

	rpmalloc_managed_heap& operator=(const rpmalloc_managed_heap& other)
	{
		if (&other == this) {
			return *this;
		}

		if (storage_) {
			storage_->decrement();
		}

		storage_ = other.use_current_heap_on_copy() ? other.storage_ : other.storage_->heap_for_copy;

		if (storage_) {
			storage_->increment();
		}
		return *this;
	}

	bool is_defined() const { return storage_ != nullptr; }

	rpmalloc_heap_t* get() const { return storage_->active; }

	rpmalloc_heap_t* copy_as() const {
		if (storage_ && storage_->heap_for_copy) {
			return storage_->heap_for_copy->active;
		}

		return nullptr;
	}

	void reset_copy() {
		if (storage_ && storage_->heap_for_copy) {
			storage_->heap_for_copy->decrement();
			storage_->heap_for_copy = nullptr;
		}
	}

	void set_heap_for_copy(const rpmalloc_managed_heap& heap) {
		if (storage_ == nullptr) {
			throw std::invalid_argument("Current smart heap is null");
		}

		if (storage_->heap_for_copy) {
			storage_->heap_for_copy->decrement();
		}

		storage_->heap_for_copy = heap.storage_;
		if (storage_->heap_for_copy) {
			storage_->heap_for_copy->increment();
		}
	}

	template<typename T, typename... Args>
	inline rpmalloc_ptr<T> make_raw(Args&&... args) const
	{
		 return rpmalloc_ptr<T>(
			new(rpmalloc_heap_alloc(storage_->active, sizeof(T))) T(std::forward<Args>(args)...));
	}

	template<typename T, typename... Args>
	inline rpmalloc_unique_ptr<T> make_unique(Args&&... args) const
	{
		return std::unique_ptr<T, rpmalloc_deleter<T>>(
			new(rpmalloc_heap_alloc(storage_->active, sizeof(T))) T(std::forward<Args>(args)...));
	}

	template<typename T, typename... Args>
	inline std::shared_ptr<T> make_shared(Args&&... args) const
	{
		return std::shared_ptr<T>(
			new(rpmalloc_heap_alloc(storage_->active, sizeof(T))) T(std::forward<Args>(args)...),
			rpmalloc_deleter<T>());
	}

	size_t get_used_size() const {
		if (storage_) {
			return rpmalloc_heap_get_used_size(storage_->active);
		}

		return 0;
	}

	size_t get_total_size() const {
		if (storage_) {
			return rpmalloc_heap_get_total_size(storage_->active);
		}

		return 0;
	}

	bool is_equal(const rpmalloc_managed_heap& other) const {
		return storage_ == other.storage_;
	}

private:
	struct storage {
		rpmalloc_heap_t* active;
		storage* heap_for_copy;

		storage()
			: active(rpmalloc_heap_acquire())
			, heap_for_copy(nullptr)
		{}

		void increment() { ++ref_count_; }

		void decrement() {
			if (--ref_count_ == 0) {
				if (heap_for_copy) {
					heap_for_copy->decrement();
				}

				rpmalloc_heap_free_all(active);
				rpmalloc_heap_release(active);
				delete this;
			}
		}

		private:
			uint64_t ref_count_ = 1;
	};

	storage* storage_ = nullptr;

	bool use_current_heap_on_copy () const { return storage_ == nullptr || storage_->heap_for_copy == nullptr; }
};

// Common base class for STL allocators in a specific heap
template<class T> struct _rp_heap_stl_allocator_common : public _rp_stl_allocator_common<T> {
  using typename _rp_stl_allocator_common<T>::size_type;
  using typename _rp_stl_allocator_common<T>::value_type;
  using typename _rp_stl_allocator_common<T>::pointer;

  _rp_heap_stl_allocator_common(const rpmalloc_managed_heap& hp)
  : heap_(hp) {}

  _rp_heap_stl_allocator_common& operator=(const _rp_heap_stl_allocator_common& other) {
		if (&other == this) {
			return *this;
		}

		heap_ = other.heap_;
		return *this;
	}

  #if (__cplusplus >= 201703L)  // C++17
  [[nodiscard]] T* allocate(size_type count) {
		return static_cast<T*>(rpmalloc_heap_calloc(heap_.get(), count, sizeof(T))); }
  [[nodiscard]] T* allocate(size_type count, const void*) { return allocate(count); }
  #else
  [[nodiscard]] pointer allocate(size_type count, const void* = 0) { return static_cast<pointer>(rpmalloc_heap_calloc(heap_.get(), count, sizeof(value_type))); }
  #endif

  #if ((__cplusplus >= 201103L) || (_MSC_VER > 1900))  // C++11
  using is_always_equal = std::false_type;
  #endif

  void collect(bool force) { }
  template<class U> bool is_equal(const _rp_heap_stl_allocator_common<U>& x) const { return heap_.is_equal(x.heap_); }

protected:
  template<class U> friend struct _rp_heap_stl_allocator_common;

	rpmalloc_managed_heap heap_;
  
  _rp_heap_stl_allocator_common() {}

  _rp_heap_stl_allocator_common(const _rp_heap_stl_allocator_common& x) noexcept : heap_(x.heap_) {}

  _rp_heap_stl_allocator_common(_rp_heap_stl_allocator_common&& x) noexcept : heap_(x.heap_) {}

  template<class U> _rp_heap_stl_allocator_common(const _rp_heap_stl_allocator_common<U>& x) noexcept : heap_(x.heap_) {}
};

// STL allocator allocation in a specific heap
template<class T> struct rp_heap_stl_allocator : public _rp_heap_stl_allocator_common<T> {
  using typename _rp_heap_stl_allocator_common<T>::size_type;
  rp_heap_stl_allocator() : _rp_heap_stl_allocator_common<T>() { }
  rp_heap_stl_allocator(const rpmalloc_managed_heap& hp) : _rp_heap_stl_allocator_common<T>(hp) { }
  template<class U> rp_heap_stl_allocator(const rp_heap_stl_allocator<U>& x) noexcept : _rp_heap_stl_allocator_common<T>(x) { }

  rp_heap_stl_allocator select_on_container_copy_construction() const { return *this; }
  void deallocate(T* p, size_type) { rpfree(p); }
  template<class U> struct rebind { typedef rp_heap_stl_allocator<U> other; };

  rp_heap_stl_allocator& operator=(const rp_heap_stl_allocator& other) {
		_rp_heap_stl_allocator_common<T>::operator=(other);
		return *this;
	}
};

template<class T1, class T2> bool operator==(const rp_heap_stl_allocator<T1>& x, const rp_heap_stl_allocator<T2>& y) noexcept { return (x.is_equal(y)); }
template<class T1, class T2> bool operator!=(const rp_heap_stl_allocator<T1>& x, const rp_heap_stl_allocator<T2>& y) noexcept { return (!x.is_equal(y)); }

#endif // C++11
#endif // RPMALLOC_FIRST_CLASS_HEAPS
#endif
