AM_CXXFLAGS = --std=c++11 -Wall
LDADD = -lgtest -lpthread
bin_PROGRAMS = unittest_all speedtest_all

unittest_all_SOURCES = test.cpp \
	util/bit_twiddling_test.cpp \
	util/bucket_search_vector_test.cpp \
	util/chunked_array_test.cpp \
	util/fake_pointer_test.cpp \
	util/immutable_skip_list_test.cpp \
	util/intseq_test.cpp \
	util/iterator_test.cpp \
	util/poly_vector_test.cpp \
	portion_test.cpp \
	chain_test.cpp \
	diag_test.cpp \
	list_test.cpp \
	bench/util/sparse_matrix_test.cpp

speedtest_all_SOURCES = test.cpp \
	util/bit_twiddling_speedtest.cpp \
	util/fake_pointer_speedtest.cpp \
	util/immutable_skip_list_speedtest.cpp \
	diag_speedtest.cpp \
	portion_speedtest.cpp
