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
#     cmake_utils_call
#     cmake_utils_call_with_var
#     cmake_utils_call_once
#     cmake_utils_set_and_make_dir

# Public globals registered by this file:
# (See ##cmake_utils_get_global and the comment
# for each of them for more detail)
#     CMAKE_GENERAL_BASE

if(COMMAND cmake_utils_call)
  return()
endif()

include(CMakeVarMacros)

function(__cmake_utils_call_helper func_name var_name)
  cmake_utils_get_unique_name(
    "___cmake_utils_call_helper_${func_name}" unique_name)
  set("${unique_name}" 0 PARENT_SCOPE)
  variable_watch("${unique_name}" "${func_name}")
  set("${var_name}" "${unique_name}" PARENT_SCOPE)
endfunction()

# cmake_utils_call(function_name)
#     @function_name: the name of function or macro to be called.
#
#     This macro will call the function or macro by name. The scope of the
#     function call is the same with the scope to call this macro therefore
#     setting variables in a macro or setting PARENT_SCOPE variables in a
#     function WILL work. This version of the function does not support
#     passing arbitrary arguments, use #cmake_utils_wrap_call,
#     #cmake_utils_call_args or #cmake_utils_call_args_with_var if you
#     need to do that.
macro(cmake_utils_call ___func_name)
  __cmake_utils_call_helper("${___func_name}"
    __cmake_utils_call_watch_var_name_${___func_name})
  set("${__cmake_utils_call_watch_var_name_${___func_name}}")
endmacro()

# cmake_utils_call_with_var(var_names function_name)
#     @var_names: the variable names to be propagate to parent scope.
#     @function_name: the name of function or macro to be called.
#
#     This functionis just like #cmake_utils_call except the macro or function
#     will be executed in a separate scope. You can control the allowed
#     variables to be passed to the caller scope using @var_names. This function
#     doesn't support calling functions or macros with arbitrary number of
#     arguments, use #cmake_utils_call_args_with_var if you need to do that.
function(cmake_utils_call_with_var varnames func_name)
  cmake_utils_call("${func_name}")
  foreach(varname ${varnames})
    set("${varname}" "${${varname}}" PARENT_SCOPE)
  endforeach()
endfunction()

# cmake_utils_call_once(function_name)
#     @function_name: the name of function or macro to be called.
#
#     This macro is just like #cmake_utils_call except it will not
#     call the function if it has already be called by this function before.
#     This is useful for a one-time global init function of a cmake module
#     that might be included more once.
#
#     NOTE: This macro does NOT interfere with #cmake_utils_call
#           and you will still be able to call the function before or after
#           calling this macro using #cmake_utils_call or directly.
macro(cmake_utils_call_once ___func_name)
  cmake_utils_check_and_set_bool(
    "__cmake_utils_call_once_${___func_name}__"
    ___cmake_utils_call_once_already_called_${___func_name})
  if("${___cmake_utils_call_once_already_called_${___func_name}}" STREQUAL 1)
    return()
  endif()
  cmake_utils_call("${___func_name}")
endmacro()

# cmake_utils_set_and_make_dir(dir global_name var_name)
#     @dir: directory name
#     @global_name: name of global to save "${dir}",
#         see #cmake_utils_get_global for more detail.
#     @var_name: name of variable to save "${dir}"
#
#     This is a helper function for cmake modules that needs working
#     directory (typically under #CMAKE_GENERAL_BASE global). This function
#     will save "${dir}" into the variable and the global as well as creating
#     the directory if it doesn't exist yet.
function(cmake_utils_set_and_make_dir dir global var)
  file(MAKE_DIRECTORY "${dir}")
  cmake_utils_set_global("${global}" "${dir}")
  set("${var}" "${dir}" PARENT_SCOPE)
endfunction()

macro(__cmake_utils_run_hook name isolate reverse)
  macro(__cmake_utils_run_hook_single)
    foreach(__cmake_utils_hook_${name} ${__cmake_utils_hooks_${name}})
      if("${isolate}")
        cmake_utils_call_with_var("" "${__cmake_utils_hook_${name}}")
      else()
        cmake_utils_call("${__cmake_utils_hook_${name}}")
      endif()
    endforeach()
  endmacro()
  foreach("__cmake_utils_hooks_${name}" "")
    if(NOT "${reverse}")
      cmake_utils_get_global("__cmake_utils_hooks_${name}"
        "__cmake_utils_hooks_${name}")
      __cmake_utils_run_hook_single()
      cmake_utils_get_global("__cmake_utils_hooks_reverse_${name}"
        "__cmake_utils_hooks_${name}")
      __cmake_utils_run_hook_single()
    else()
      macro(__cmake_utils_run_hook_single_reverse)
        if(NOT "x${__cmake_utils_hooks_${name}}x" STREQUAL xx)
          list(REVERSE "__cmake_utils_hooks_${name}")
          __cmake_utils_run_hook_single()
        endif()
      endmacro()
      cmake_utils_get_global("__cmake_utils_hooks_reverse_${name}"
        "__cmake_utils_hooks_${name}")
      __cmake_utils_run_hook_single_reverse()
      cmake_utils_get_global("__cmake_utils_hooks_${name}"
        "__cmake_utils_hooks_${name}")
      __cmake_utils_run_hook_single_reverse()
    endif()
  endforeach()
endmacro()

function(_cmake_utils_run_hook_isolated name reverse)
  __cmake_utils_run_hook("${name}" True "${reverse}")
endfunction()

macro(cmake_utils_run_hook name reverse)
  if("${ARGC}" LESS 3 OR NOT "${ARGV2}")
    __cmake_utils_run_hook("${name}" False "${reverse}")
  elseif("${ARGV2}" STREQUAL 2)
    _cmake_utils_run_hook_isolated("${name}" "${reverse}")
  else()
    __cmake_utils_run_hook("${name}" True "${reverse}")
  endif()
endmacro()

function(cmake_utils_reg_hook name func)
  if("${ARGC}" LESS "3" OR NOT "${ARGV2}")
    cmake_utils_get_global("__cmake_utils_hooks_${name}" hooks)
    cmake_utils_set_global("__cmake_utils_hooks_${name}" "${hooks};${func}")
  else()
    cmake_utils_get_global("__cmake_utils_hooks_reverse_${name}" hooks)
    cmake_utils_set_global("__cmake_utils_hooks_reverse_${name}"
      "${func};${hooks}")
  endif()
endfunction()

function(cmake_utils_config_end_hook func)
  cmake_utils_reg_hook("__cmake_utils_end_of_configure_hook"
    "${func}" ${ARGN})
endfunction()

function(___cmake_utils_parent_file_hook var acc value cur_lst stack)
  if(NOT "x${value}x" STREQUAL xx)
    return()
  endif()
  cmake_utils_run_hook("__cmake_utils_end_of_configure_hook" False 2)
endfunction()

# CMAKE_GENERAL_BASE
#     This global contains the path to a hidden directory the binary directory
#     that can be used as a base directory for saving building files.
function(__cmake_base_utils_init)
  cmake_utils_set_and_make_dir("${CMAKE_BINARY_DIR}/.cmake_utils_base"
    CMAKE_GENERAL_BASE base_dir)
  variable_watch(CMAKE_PARENT_LIST_FILE ___cmake_utils_parent_file_hook)
endfunction()
cmake_utils_call_once(__cmake_base_utils_init)
