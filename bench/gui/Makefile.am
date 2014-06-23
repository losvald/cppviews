AM_CXXFLAGS = --std=c++11 -pedantic -Wall $(WX_CXXFLAGS)
LDADD = $(WX_LIBS)
bin_PROGRAMS = smvd

# Sparse Matrix View Designer
smvd_SOURCES = smvd.cpp \
	smvd_display.cpp \
	smvd_frame.cpp \
	smvd_navigator.cpp \
	sm_displayable.cpp \
	sm/view_tree.cpp \
	sm/view_type.cpp \
	gen/smvd_gui.cpp
