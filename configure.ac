#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(xgtools, 1.0, [j.pickering@imperial.ac.uk])
AC_CONFIG_SRCDIR([src/xgline.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([gslcblas], cblas_strsv, [],[
	echo "The GNU Scientific Library (GSL) is required to compile FAST"
	exit -1])
# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([pow sqrt])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
