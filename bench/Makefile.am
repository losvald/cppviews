AM_CXXFLAGS = --std=c++11 -Wall
bin_PROGRAMS = mm mm_views mm_checker

VIEWS_LDADD = ../src/libcppviews.a
mm_SOURCES = mm.cpp
mm_views_SOURCES = mm_views.cpp
mm_views_LDADD = $(VIEWS_LDADD)
mm_checker_SOURCES = mm_checker.cpp
