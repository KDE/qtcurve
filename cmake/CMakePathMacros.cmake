# Copyright (C) 2013~2014 by Yichao Yu
# yyc1992@gmail.com
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Public functions and macros provided by this file
# (See comment of each for more detail):

include(CMakeArrayMacros)

function(cmake_utils_abs_path var path)
  get_filename_component(abs_path "${path}" ABSOLUTE)
  if("x${abs_path}x" MATCHES "^x//")
    string(SUBSTRING "${abs_path}" 1 -1 abs_path)
  endif()
  set("${var}" "${abs_path}" PARENT_SCOPE)
endfunction()

macro(__cmake_utils_to_abs_foreach)
  if("x${${__cmake_utils_to_abs_path_value}}x" STREQUAL xx)
    return()
  endif()
  cmake_utils_abs_path("${__cmake_utils_to_abs_path_value}"
    "${${__cmake_utils_to_abs_path_value}}")
  set("${__cmake_utils_to_abs_path_value}"
    "${${__cmake_utils_to_abs_path_value}}" PARENT_SCOPE)
endmacro()

function(cmake_utils_to_abs)
  cmake_array_foreach(__cmake_utils_to_abs_path_value
    __cmake_utils_to_abs_foreach)
endfunction()

function(cmake_utils_is_subpath ret_var parent child)
  cmake_utils_to_abs(parent child)
  file(RELATIVE_PATH rel_path "${parent}" "${child}")
  string(REGEX MATCH "^\\.\\./" match "${rel_path}")
  if(match)
    set("${ret_var}" False PARENT_SCOPE)
  else()
    set("${ret_var}" True PARENT_SCOPE)
  endif()
endfunction()

function(__cmake_utils_src_to_bin_with_path out path src_path bin_path)
  cmake_utils_is_subpath(issub "${src_path}" "${path}")
  if(issub)
    file(RELATIVE_PATH rel_path "${src_path}" "${path}")
    cmake_utils_abs_path(path "${bin_path}/${rel_path}")
    set("${out}" "${path}" PARENT_SCOPE)
  endif()
endfunction()

function(cmake_utils_src_to_bin out path)
  set(bin_path)
  __cmake_utils_src_to_bin_with_path(bin_path "${path}"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
  if(NOT "x${bin_path}" STREQUAL "x")
    set("${out}" "${bin_path}" PARENT_SCOPE)
    return()
  endif()
  __cmake_utils_src_to_bin_with_path(bin_path "${path}"
    "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
  if(NOT "x${bin_path}" STREQUAL "x")
    set("${out}" "${bin_path}" PARENT_SCOPE)
    return()
  endif()
  __cmake_utils_src_to_bin_with_path(bin_path "${path}"
    "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
  if(NOT "x${bin_path}" STREQUAL "x")
    set("${out}" "${bin_path}" PARENT_SCOPE)
    return()
  endif()
  set("${out}" "${path}" PARENT_SCOPE)
endfunction()

function(_cmake_utils_std_fname_with_dirs var bin src fname)
  cmake_utils_is_subpath(is_bin "${bin}" "${fname}")
  cmake_utils_is_subpath(is_src "${src}" "${fname}")
  if(is_bin OR is_src)
    if(NOT is_src)
      file(RELATIVE_PATH rel_path "${bin}" "${fname}")
    elseif(NOT is_bin)
      file(RELATIVE_PATH rel_path "${src}" "${fname}")
    else()
      cmake_utils_is_subpath(bin_in_src "${src}" "${bin}")
      if(bin_in_src)
        file(RELATIVE_PATH rel_path "${bin}" "${fname}")
      else()
        file(RELATIVE_PATH rel_path "${src}" "${fname}")
      endif()
    endif()
    set("${var}" "${rel_path}" PARENT_SCOPE)
  else()
    set("${var}" "" PARENT_SCOPE)
  endif()
endfunction()

function(cmake_utils_std_fname var fname)
  cmake_utils_to_abs(fname)
  _cmake_utils_std_fname_with_dirs(cur_rel_path "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${fname}")
  if(NOT "x${cur_rel_path}x" STREQUAL "xx")
    set("${var}" "${cur_rel_path}" PARENT_SCOPE)
    return()
  endif()
  _cmake_utils_std_fname_with_dirs(pro_rel_path "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}" "${fname}")
  if(NOT "x${pro_rel_path}x" STREQUAL "xx")
    set("${var}" "${pro_rel_path}" PARENT_SCOPE)
    return()
  endif()
  _cmake_utils_std_fname_with_dirs(cmake_rel_path "${CMAKE_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}" "${fname}")
  if(NOT "x${cmake_rel_path}x" STREQUAL "xx")
    set("${var}" "${cmake_rel_path}" PARENT_SCOPE)
    return()
  endif()
  set("${var}" "${fname}" PARENT_SCOPE)
endfunction()
