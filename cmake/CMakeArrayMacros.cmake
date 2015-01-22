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
#     cmake_array_foreach
#     cmake_array_copy
#     cmake_array_propagate_name
#     cmake_array_new
#     cmake_array_new_len
#     cmake_array_append
#     cmake_array_clear
#     cmake_array_concat
#     cmake_array_slice
#     cmake_array_set_global_single
#     cmake_array_set_global
#     cmake_array_get_global

if(COMMAND cmake_array_foreach)
  return()
endif()

include(CMakeBaseMacros)

macro(cmake_array_defined array res)
  if(DEFINED "${array}.length" AND
      "x${${array}.length}x" MATCHES "^x(0|[1-9][0-9]*)x\$")
    set("${res}" True)
  else()
    set("${res}" False)
  endif()
endmacro()

function(cmake_array_abort_undefined array)
  cmake_array_defined("${array}" defined)
  if(NOT defined)
    message(FATAL_ERROR "Array ${array} is not defined.")
  endif()
endfunction()

macro(__cmake_array_foreach_real vname fname prefix start stop)
  foreach(__cmake_array_foreach_${vname}_${fname}_i RANGE
      "${start}" "${stop}")
    if("${__cmake_array_foreach_${vname}_${fname}_i}" STREQUAL "${stop}")
      break()
    endif()
    # use foreach instead of set to prevent variable ${vname} leaking to
    # caller scope.
    foreach("${vname}"
        "${${prefix}${__cmake_array_foreach_${vname}_${fname}_i}}")
      cmake_utils_call("${fname}")
    endforeach()
  endforeach()
endmacro()

# cmake_array_foreach(varname funcname [prefix] [start | stop | start stop])
#     @varname: name of the loop variable.
#     @funcname: name of the function or macro to be run in each loop.
#     @prefix: name of the array. default: ARGV
#     @start: the start index. default: 0
#     @stop: the stop index. Default to the length of the array. For ARGV this
#         is ARGC, otherwise it is @prefix.length
#
#     This macro run @funcname on a variable list with variable @varname
#     set to the value of each variable. A variable list is a set of variables
#     with names prefix+number. The prefix is @prefix and the range of the
#     number is from @start to (@stop - 1). This can work on the arguments
#     of a function but CANNOT work on arguments of a macro because they are
#     not cmake variables.
macro(cmake_array_foreach varname funcname)
  if("${ARGC}" STREQUAL "2")
    foreach(__cmake_array_foreach_${varname}_${funcname}_argc ARGC)
      __cmake_array_foreach_real("${varname}" "${funcname}"
        ARGV 0 "${${__cmake_array_foreach_${varname}_${funcname}_argc}}")
    endforeach()
  elseif("${ARGC}" STREQUAL "3")
    if("${ARGV2}" MATCHES "^[0-9]+\$")
      foreach(__cmake_array_foreach_${varname}_${funcname}_argc ARGC)
        __cmake_array_foreach_real("${varname}" "${funcname}"
          ARGV "${ARGV2}"
          "${${__cmake_array_foreach_${varname}_${funcname}_argc}}")
      endforeach()
    else()
      if("x${ARGV2}" STREQUAL "xARGV")
        foreach(__cmake_array_foreach_${varname}_${funcname}_argc ARGC)
          __cmake_array_foreach_real("${varname}" "${funcname}"
            ARGV 0 "${${__cmake_array_foreach_${varname}_${funcname}_argc}}")
        endforeach()
      else()
        cmake_array_abort_undefined("${ARGV2}")
        __cmake_array_foreach_real("${varname}" "${funcname}"
          "${ARGV2}." 0 "${${ARGV2}.length}")
      endif()
    endif()
  elseif("${ARGC}" STREQUAL "4")
    if("${ARGV2}" MATCHES "^[0-9]+\$")
      __cmake_array_foreach_real("${varname}" "${funcname}"
        ARGV "${ARGV2}" "${ARGV3}")
    else()
      cmake_array_abort_undefined("${ARGV2}")
      __cmake_array_foreach_real("${varname}" "${funcname}"
        "${ARGV2}." 0 "${ARGV3}")
    endif()
  elseif("${ARGC}" STREQUAL "5")
    cmake_array_abort_undefined("${ARGV2}")
    __cmake_array_foreach_real("${varname}" "${funcname}"
      "${ARGV2}." "${ARGV3}" "${ARGV4}")
  else()
    message(FATAL_ERROR "cmake_array_foreach called with wrong number of argument ${ARGC}. Expect at most 5.")
  endif()
endmacro()

macro(_cmake_array_new_name ret)
  cmake_utils_get_unique_name(cmake_array_name "${ret}")
endmacro()

# cmake_array_copy(from to [parent_scope])
#     @from: name of the source array.
#     @to: name of the destination array.
#     @parent_scope: whether copy this array to parent scope.
#
#     This macro copies the @from array to @to. If @parent_scope is #true,
#     the array will also be copied to parent scope.
macro(cmake_array_copy from to)
  cmake_array_new("${to}")
  if("${ARGC}" GREATER 2 AND "${ARGV2}")
    _cmake_array_copy("${from}" "${${to}}" PARENT_SCOPE)
  endif()
  _cmake_array_copy("${from}" "${${to}}")
endmacro()

macro(_cmake_array_copy from to)
  cmake_array_abort_undefined("${from}")
  cmake_array_abort_undefined("${to}")
  set("${to}.length" "${${from}.length}" ${ARGN})
  # It doesn't matter to propagate one element more.
  foreach(__cmake_array_copy_i RANGE "${${from}.length}")
    set("${to}.${__cmake_array_copy_i}"
      "${${from}.${__cmake_array_copy_i}}" ${ARGN})
  endforeach()
endmacro()

# cmake_array_propagate_name(arrarname)
#     @arrayname: name of the array to be propagate to parent scope.
#
#     This macro propagate the array @arrayname to the parent scope of the
#     caller. Use this to return a array.
macro(cmake_array_propagate name)
  _cmake_array_copy("${name}" "${name}" PARENT_SCOPE)
endmacro()

macro(__cmake_array_new_foreach)
  set("${new_array_name}.${i}" "${__cmake_array_new_value}" PARENT_SCOPE)
  math(EXPR i "${i} + 1")
endmacro()

# cmake_array_new(varname [elements...])
#     @varname: the name of the array to be created.
#     @elements: elements of the array.
#
#     This function create a new array of name @varname from the argument of
#     the function (@elements) in the current scope.
function(cmake_array_new varname)
  _cmake_array_new_name(new_array_name)
  math(EXPR length "${ARGC} - 1")
  set("${new_array_name}.length" "${length}" PARENT_SCOPE)
  set(i 0)
  cmake_array_foreach(__cmake_array_new_value __cmake_array_new_foreach 1)
  set("${varname}" "${new_array_name}" PARENT_SCOPE)
endfunction()

macro(__cmake_array_new_len_foreach)
  math(EXPR length "${i} + 1")
  if(length GREATER len)
    return()
  endif()
  set("${new_array_name}.${i}" "${__cmake_array_new_len_value}" PARENT_SCOPE)
  math(EXPR i "${length}")
endmacro()

# cmake_array_new_len(varname len [elements...])
#     @varname: the name of the array to be created.
#     @len: the maximum length of the array.
#     @elements: elements of the array.
#
#     This function create a new array of name @varname from the argument of
#     the function (@elements) in the current scope. At most @len elements
#     will be added to the array.
function(cmake_array_new_len varname len)
  _cmake_array_new_name(new_array_name)
  math(EXPR maxidx "${len} - 1")
  set(i 0)
  set(length 0)
  cmake_array_foreach(__cmake_array_new_len_value
    __cmake_array_new_len_foreach 1)
  set("${new_array_name}.length" "${length}" PARENT_SCOPE)
  set("${varname}" "${new_array_name}" PARENT_SCOPE)
endfunction()

macro(__cmake_array_append_foreach)
  set("${varname}.${${varname}.length}" "${element}")
  math(EXPR "${varname}.length" "${${varname}.length} + 1")
endmacro()

# cmake_array_append(varname [elements...])
#     @varname: name of the array.
#     @elements: elements to be appended to the array.
#
#     This function appends @elements to @varname.
function(cmake_array_append varname)
  cmake_array_abort_undefined("${varname}")
  cmake_array_foreach(element __cmake_array_append_foreach 1)
  cmake_array_propagate("${varname}")
endfunction()

# cmake_array_clear(varname)
#     @varname: name of the array to be cleared.
#
#     This function clears array @varname.
function(cmake_array_clear varname)
  cmake_array_abort_undefined("${varname}")
  set("${varname}.length" "0" PARENT_SCOPE)
endfunction()

# cmake_array_concat(varname [prefix] [start | stop | start stop])
#     @varname: name of the destination array.
#     @prefix: name of the source array. default: ARGV
#     @start: the start index. default: 0
#     @stop: the stop index. Default to length of the array.
#
#     This macro append the array @prefix to array @varname. @prefix can be
#     the argument list of a function. Defaults are treated the same with
#     #cmake_array_foreach. See #cmake_array_foreach for more detail.
macro(cmake_array_concat varname)
  # Lexical scope used here.
  # Do not move the definition out of the parent macro.
  macro(__cmake_array_concat_foreach)
    cmake_array_append("${varname}" "${__cmake_array_concat_value}")
  endmacro()
  cmake_array_foreach(__cmake_array_concat_value
    __cmake_array_concat_foreach ${ARGN})
endmacro()

# cmake_array_slice(varname [prefix] [start | stop | start stop])
#     @varname: name of the destination array.
#     @prefix: name of the source array. default: ARGV
#     @start: the start index. default: 0
#     @stop: the stop index. Default to length of the array.
#
#     This macro create a new array @varname from array @prefix between
#     @start and @stop. See #cmake_array_foreach for more detail.
macro(cmake_array_slice varname)
  cmake_array_new("${varname}")
  cmake_array_concat("${${varname}}" ${ARGN})
endmacro()

# cmake_array_set_global_single(global [prefix] [start | stop | start stop])
#     @global: the name of the global to save the array.
#     @prefix: name of the source array. default: ARGV
#     @start: the start index. default: 0
#     @stop: the stop index. Default to length of the array.
#
#     This macro saves the array to @global. For more detail about @prefix,
#     @start and @stop, see #cmake_array_foreach.
#     See also #cmake_array_set_global and #cmake_array_get_global.
macro(cmake_array_set_global_single global)
  macro(__cmake_array_set_global_foreach)
    cmake_utils_set_global("${global}.${__cmake_array_set_global_i}"
      "${__cmake_array_set_global_value}")
    math(EXPR __cmake_array_set_global_i "${__cmake_array_set_global_i} + 1")
  endmacro()
  foreach(__cmake_array_set_global_i 0)
    cmake_array_foreach(__cmake_array_set_global_value
      __cmake_array_set_global_foreach ${ARGN})
    cmake_utils_set_global("${global}.length" "${__cmake_array_set_global_i}")
  endforeach()
endmacro()

function(_cmake_array_get_global_single global array)
  cmake_utils_get_global("${global}.length" "${array}.length")
  foreach(array_i RANGE 0 "${${array}.length}")
    if("x${${array}.length}x" STREQUAL "x${array_i}x")
      break()
    endif()
    cmake_utils_get_global("${global}.${array_i}" "${array}.${array_i}")
  endforeach()
  cmake_array_propagate("${array}")
endfunction()

macro(__cmake_array_set_global_rec_foreach)
  foreach(global "${global}.${__cmake_array_i}")
    foreach(array "${__cmake_array_value}")
      _cmake_array_set_global_rec("${global}" "${array}" "${level}")
    endforeach()
  endforeach()
  math(EXPR __cmake_array_i "${__cmake_array_i} + 1")
endmacro()

function(_cmake_array_set_global_rec global array level)
  if("${level}" LESS 2)
    cmake_array_set_global_single("${global}" "${array}")
    return()
  endif()
  math(EXPR level "${level} - 1")
  cmake_utils_set_global("${global}.length" "${${array}.length}")
  foreach(__cmake_array_i 0)
    cmake_array_foreach(__cmake_array_value
      __cmake_array_set_global_rec_foreach "${array}")
  endforeach()
endfunction()

macro(__cmake_array_get_global_rec global array level)
  if("${level}" LESS 2)
    _cmake_array_get_global_single("${global}" "${array}")
  else()
    math(EXPR new_level "${level} - 1")
    cmake_utils_get_global("${global}.length" "${array}.length")
    foreach(array_i RANGE 0 "${${array}.length}")
      if("x${${array}.length}x" STREQUAL "x${array_i}x")
        break()
      endif()
      __cmake_array_get_global_rec("${global}.${array_i}"
        "${array}.${array_i}" "${new_level}")
      set("${array}.${array_i}" "${array}.${array_i}")
    endforeach()
  endif()
  cmake_array_propagate("${array}")
endmacro()

function(_cmake_array_get_global_rec global array level)
  __cmake_array_get_global_rec("${global}" "${array}" "${level}")
endfunction()

# cmake_array_get_global(global array [level])
#     @global: the name of the global to save the array.
#     @array: the name of the array.
#     @level: level of recursion. Default to 1.
#
#     This macro retrieves a multi-level array that is previously
#     saved as @global to @array.
#     See also #cmake_array_set_global and #cmake_array_set_global_single.
macro(cmake_array_get_global global array)
  _cmake_array_new_name("${array}")
  if("${ARGC}" GREATER 2)
    _cmake_array_get_global_rec("${global}" "${${array}}" "${ARGV2}")
  else()
    _cmake_array_get_global_single("${global}" "${${array}}")
  endif()
endmacro()

# cmake_array_set_global(global array [level])
#     @global: the name of the global to save the array.
#     @array: the name of the array.
#     @level: level of recursion. Default to 1.
#
#     This macro saves the multi-level @array as @global.
#     See also #cmake_array_get_global and #cmake_array_set_global_single.
macro(cmake_array_set_global global array)
  if("${ARGC}" GREATER 2)
    _cmake_array_set_global_rec("${global}" "${array}" "${ARGV2}")
  else()
    cmake_array_set_global_single("${global}" "${array}")
  endif()
endmacro()

function(cmake_array_insert array index)
  cmake_array_slice(array_insert_after "${array}" "${index}"
    "${${array}.length}")
  cmake_array_slice(array_insert_before "${array}" 0 "${index}")
  cmake_array_concat("${array_insert_before}" 2)
  cmake_array_concat("${array_insert_before}" "${array_insert_after}")
  _cmake_array_copy("${array_insert_before}" "${array}" PARENT_SCOPE)
endfunction()

function(cmake_array_pop)
endfunction()

function(cmake_array_remove)
endfunction()

function(cmake_array_reverse)
endfunction()
