# Try to find Libexecinfo functionality
# Once done this will define
#
#  LIBEXECINFO_FOUND - system has Libexecinfo
#  LIBEXECINFO_INCLUDE_DIR - Libexecinfo include directory
#  LIBEXECINFO_LIBRARIES - Libraries needed to use Libexecinfo
#
# TODO: This will enable translations only if Gettext functionality is
# present in libc. Must have more robust system for release, where Gettext
# functionality can also reside in standalone Gettext library, or the one
# embedded within kdelibs (cf. gettext.m4 from Gettext source).

# Copyright (c) 2006, Chusslove Illich, <caslav.ilic@gmx.net>
# Copyright (c) 2007, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBEXECINFO_INCLUDE_DIR AND LIBEXECINFO_LIB_FOUND)
  set(Libexecinfo_FIND_QUIETLY TRUE)
endif()

unset(LIBEXECINFO_INCLUDE_DIR CACHE)
find_path(LIBEXECINFO_INCLUDE_DIR execinfo.h)

set(LIBEXECINFO_LIB_FOUND FALSE)

if(LIBEXECINFO_INCLUDE_DIR)
  include(CheckFunctionExists)
  check_function_exists(backtrace LIBEXECINFO_LIBC_HAS_LIBEXECINFO_BACKTRACE)

  if(LIBEXECINFO_LIBC_HAS_LIBEXECINFO_BACKTRACE)
    set(LIBEXECINFO_LIBRARIES)
    set(LIBEXECINFO_LIB_FOUND TRUE)
  else()
    unset(LIBEXECINFO_LIBRARIES CACHE)
    find_library(LIBEXECINFO_LIBRARIES NAMES execinfo libexecinfo)
    if(LIBEXECINFO_LIBRARIES)
      set(LIBEXECINFO_LIB_FOUND TRUE)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libexecinfo
  DEFAULT_MSG LIBEXECINFO_INCLUDE_DIR LIBEXECINFO_LIB_FOUND)
mark_as_advanced(LIBEXECINFO_INCLUDE_DIR LIBEXECINFO_LIBRARIES
  LIBEXECINFO_LIBC_HAS_LIBEXECINFO_BACKTRACE LIBEXECINFO_LIB_FOUND)
