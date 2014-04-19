#ifndef CPPVIEWS_BENCH_HPCC_RANDOMACCESS_VIEW_H_
#define CPPVIEWS_BENCH_HPCC_RANDOMACCESS_VIEW_H_

#ifndef RA_VIEW_
#include "../../../src/list.hpp"
#include "allocator.h"
#endif  // RA_VIEW_

#ifdef RA_VIEW_CHUNKED
#include "../../../src/util/chunked_array.hpp"

#ifndef RA_CHUNK_SIZE_LOG
#define RA_CHUNK_SIZE_LOG 10
#endif  // !defined(RA_CHUNK_SIZE_LOG)

typedef ChunkedArray<u64Int, RA_CHUNK_SIZE_LOG, 0,
                     HpccXAllocator<u64Int> > ChunkedView;
#endif  // defined(RA_VIEW_CHUNKED)

#endif  /* CPPVIEWS_BENCH_HPCC_RANDOMACCESS_VIEW_H_ */
