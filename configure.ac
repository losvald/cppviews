AC_INIT([CppViews], [0.1], [leo.osvald@gmail.com],
	[cppviews], [https://bitbucket.org/losvald/cppviews])
AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.10 foreign -Wall no-define])
AC_PROG_CXX
AC_PROG_RANLIB
LT_INIT
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile test/Makefile bench/Makefile \
	bench/gui/Makefile])

# Check for wxWidgets version >= 3 (required for test/gui)
AM_OPTIONS_WXCONFIG
reqwx=3.0.0
AM_PATH_WXCONFIG($reqwx, wxWin=1)
if test "$wxWin" != 1; then
  AC_MSG_ERROR([
	wxWidgets must be installed on your system.

	Please check that wx-config is in path, the directory where
	wxWidgets libraries are installed (returned by
	'wx-config --libs' or 'wx-config --static --libs'
	command) is in LD_LIBRARY_PATH or equivalent variable
	and wxWidgets version is $reqwx or above.
  ])
fi

AC_LANG(C++)
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <gtest/gtest.h>],
    [])],
  [TEST_LIBS="$TEST_LIBS -lgtest"] [HAVE_GTEST=1],
  [AC_MSG_ERROR([libgtest is not installed.])])
AC_OUTPUT
