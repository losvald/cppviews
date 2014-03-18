AM_CXXFLAGS = --std=c++11 -Wall
bin_PROGRAMS = unittest_all speedtest_all

unittest_all_SOURCES = test.cpp \
	portion_test.cpp

speedtest_all_SOURCES = test.cpp \
	portion_speedtest.cpp

unittest_all_LDADD = ../src/libcppviews.a -lgtest -lpthread
speedtest_all_LDADD = ../src/libcppviews.a -lgtest -lpthread
