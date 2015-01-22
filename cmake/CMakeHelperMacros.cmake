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
#     cmake_utils_reconf_on
#     cmake_helper_clear_cache
#     cmake_helper_reg
#     cmake_helper_wrap_shell
#     cmake_helper_wrap_helper
#     cmake_helper_add_command
#     cmake_helper_set_command
#     cmake_helper_add_env

if(COMMAND cmake_helper_reg)
  return()
endif()

include(CMakeArgumentMacros)
include(CMakeStringMacros)
include(CMakePathMacros)

## reconfigure
macro(__cmake_utils_reconf_on_foreach)
  string(MD5 fname "${__cmake_utils_reconf_on_value}")
  configure_file("${__cmake_utils_reconf_on_value}"
    "${reconf_cache}/${fname}" COPYONLY)
  # file(REMOVE "${reconf_cache}/${fname}")
endmacro()

# cmake_utils_reconf_on([filenames...])
#     @filenames: names of files to trigger reconfigure when changed.
#
#     This function add @filenames to the configure dependencies so that
#     the configure stage (i.e. the cmake scripts) will be run again if these
#     files are modified. This is useful if the file is read/parsed/executed
#     in the configure stage which may produce a different configure result
#     when changed.
function(cmake_utils_reconf_on)
  cmake_utils_get_global(___CMAKE_UTILS_RECONF_CACHE reconf_cache)
  cmake_array_foreach(__cmake_utils_reconf_on_value
    __cmake_utils_reconf_on_foreach)
endfunction()

## clear cache
macro(__cmake_helper_clear_cache_foreach)
  execute_process(COMMAND "${helper_wrapper}"
    --clear-cache "${__cmake_helper_clear_cache_value}")
endmacro()

# cmake_helper_clear_cache([dirs...])
#     @dirs: directories names to be cleared.
#
#     This function removes the content and recreate @dirs.
#     Used for cache dir cleanup.
function(cmake_helper_clear_cache)
  cmake_utils_get_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER helper_wrapper)
  cmake_array_foreach(__cmake_helper_clear_cache_value
    __cmake_helper_clear_cache_foreach)
endfunction()

## write shell script helpers
### init
macro(__cmake_helper_init_script_foreach)
  file(WRITE "${fname}"
    "#!/bin/sh\n# Automatically generated. DO NOT EDIT\n")
endmacro()

function(_cmake_helper_init_script)
  cmake_array_foreach(fname __cmake_helper_init_script_foreach)
endfunction()

### command
macro(__cmake_helper_escape_cmd var)
  macro(__cmake_helper_escape_cmd_foreach)
    cmake_str_quote_shell("${arg}" arg)
    if(_first_arg)
      set("${var}" "${${var}}${arg}")
      set(_first_arg False)
    else()
      set("${var}" "${${var}} ${arg}")
    endif()
  endmacro()
  foreach(_first_arg True)
    cmake_array_foreach(arg __cmake_helper_escape_cmd_foreach ${ARGN})
  endforeach()
endmacro()

function(_cmake_helper_escape_cmd var)
  set(escaped_cmd "${${var}}")
  __cmake_helper_escape_cmd(escaped_cmd 1)
  set("${var}" "${escaped_cmd}" PARENT_SCOPE)
endfunction()

macro(__cmake_helper_escape_envs varname)
  macro(__cmake_helper_escape_envs_foreach)
    if(__cmake_helper_add_env_is_name)
      set(__cmake_helper_add_env_is_name 0)
      set(__cmake_helper_add_env_name "${__cmake_helper_escape_envs_value}")
    else()
      set(__cmake_helper_add_env_is_name 1)
      _cmake_helper_escape_cmd("${varname}" export
        "${__cmake_helper_add_env_name}=${__cmake_helper_escape_envs_value}")
      set("${varname}" "${${varname}}\n")
    endif()
  endmacro()
  foreach(__cmake_helper_add_env_is_name 1)
    foreach(__cmake_helper_add_env_name "")
      cmake_array_foreach(__cmake_helper_escape_envs_value
        __cmake_helper_escape_envs_foreach ${ARGN})
    endforeach()
  endforeach()
endmacro()

function(_cmake_helper_write_export file)
  set(escaped_envs)
  __cmake_helper_escape_envs(escaped_envs 1)
  file(APPEND "${file}" "${escaped_envs}")
endfunction()

macro(__cmake_helper_wrap_shell command_ret)
  __cmake_helper_escape_cmd(quoted_command_line ${ARGN})
  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  cmake_utils_get_global(_CMAKE_HELPER_SCRIPT_DIR
    cmake_run_helper_script_dir)
  cmake_utils_get_unique_name(cmake_helper_wrap_shell "${command_ret}")
  set("${command_ret}" "${helper_cache}/${${command_ret}}.sh")
  set(quoted_command_line "${quoted_command_line} \"\$@\"")
  configure_file(
    "${cmake_run_helper_script_dir}/cmake-utils-cmd-wrapper.sh.in"
    "${${command_ret}}" @ONLY)
  cmake_utils_reconf_on("${${command_ret}}")
endmacro()

function(_cmake_helper_wrap_shell command_ret)
  __cmake_helper_wrap_shell(file_name 1)
  set("${command_ret}" "${file_name}" PARENT_SCOPE)
endfunction()

### variables
function(__cmake_helper_write_vars fname name value)
  cmake_str_quote_shell("${value}" value)
  file(APPEND "${fname}" "${name}=${value}\n")
endfunction()

### execute with environment
macro(____cmake_helper_wrap_commands_foreach)
  foreach(command_str "")
    __cmake_helper_escape_cmd(command_str "${command}")
    file(APPEND "${fname}" "    ${command_str} \"$@\"\n")
  endforeach()
endmacro()

function(__cmake_helper_wrap_commands_write_commands fname name commands)
  file(APPEND "${fname}" "__command_${name}_run() {\n")
  cmake_array_foreach(command ____cmake_helper_wrap_commands_foreach
    "${commands}")
  file(APPEND "${fname}" "}\n")
endfunction()

function(__cmake_helper_wrap_commands env_fname name
    commands working_dir input output error envs)
  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  set(fname "${helper_cache}/command_${name}_setting.sh")
  _cmake_helper_init_script("${fname}")
  # envs
  if(NOT "x${env_fname}x" STREQUAL xx)
    if((NOT "x${envs}x" STREQUAL xx) AND DEFINED "${envs}.length"
        AND "${${envs}.length}")
      __cmake_helper_escape_envs(envs_string "${envs}")
      file(APPEND "${env_fname}" "${envs_string}")
    endif()
    __cmake_helper_write_vars("${fname}" "__command_${name}_env_file"
      "${env_fname}")
  endif()
  # settings
  __cmake_helper_write_vars("${fname}" "__command_${name}_input_file"
    "${input}")
  __cmake_helper_write_vars("${fname}" "__command_${name}_output_file"
    "${output}")
  __cmake_helper_write_vars("${fname}" "__command_${name}_error_file"
    "${error}")
  __cmake_helper_write_vars("${fname}" "__command_${name}_working_dir"
    "${working_dir}")
  # commands
  __cmake_helper_wrap_commands_write_commands("${fname}" "${name}"
    "${commands}")
  cmake_utils_reconf_on("${fname}")
endfunction()

function(cmake_helper_to_cur_file abs_path file)
  if(IS_ABSOLUTE "${file}")
    set("${abs_path}" "${file}" PARENT_SCOPE)
  else()
    get_filename_component(list_file_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
    set(file_path "${list_file_dir}/${file}")
    cmake_utils_to_abs(file_path)
    set("${abs_path}" "${file_path}" PARENT_SCOPE)
  endif()
endfunction()

function(__cmake_helper_macro_init)
  cmake_utils_get_global(CMAKE_GENERAL_BASE base_dir)

  # helper cache
  cmake_utils_set_and_make_dir("${base_dir}/cmake_helper_cache"
    ___CMAKE_HELPER_UTILS_CACHE helper_cache)
  get_filename_component(cmake_run_helper_script_dir
    "${CMAKE_CURRENT_LIST_FILE}" PATH)
  set(cmake_run_helper_script
    "${cmake_run_helper_script_dir}/cmake-utils-run-helper.sh")
  cmake_str_quote_shell("${helper_cache}" cmake_helper_utils_cache_quoted)
  cmake_str_quote_shell("${cmake_run_helper_script_dir}"
    cmake_helper_script_dir_quoted)
  cmake_str_quote_shell("${base_dir}" base_dir_quoted)
  set(cmake_run_helper_wrapper "${base_dir}/cmake-utils-run-helper-wrapper.sh")
  configure_file(
    "${cmake_run_helper_script_dir}/cmake-utils-run-helper-wrapper.sh.in"
    "${cmake_run_helper_wrapper}" @ONLY)
  cmake_utils_set_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER
    "${cmake_run_helper_wrapper}")
  cmake_utils_set_global(_CMAKE_HELPER_SCRIPT_DIR
    "${cmake_run_helper_script_dir}")
  set(general_export_script "${base_dir}/cmake_helper_general_export.sh")
  _cmake_helper_init_script("${general_export_script}")
  _cmake_helper_write_export("${general_export_script}"
    CMAKE_HELPER_CMAKE_COMMAND "${CMAKE_COMMAND}")
  cmake_helper_clear_cache("${helper_cache}")

  # reconfigure
  cmake_utils_set_and_make_dir("${base_dir}/cmake_reconf_cache"
    ___CMAKE_UTILS_RECONF_CACHE reconf_cache)
  cmake_helper_clear_cache("${reconf_cache}")
  cmake_utils_reconf_on("${cmake_run_helper_script}")
  cmake_utils_reconf_on("${cmake_run_helper_wrapper}")
  cmake_utils_reconf_on("${general_export_script}")
endfunction()
cmake_utils_call_once(__cmake_helper_macro_init)

# cmake_helper_reg(name file command_ret
#                  [ENVS:] (env_name env_value)...
#                  WORKING_DIR: working_directory)
#     @name: name of the helper.
#     @file: executable file name of the helper. Relative path
#         is resolved using the directory of the current script.
#     @command_ret: variable to return the wrapper command used to
#         run the helper.
#     @env_name: name of environment variable.
#     @env_value: value of environment variable.
#     @working_directory: working directory to run the helper.
#         default to the directory of the current script.
#
#     This function registers a helper script @file as @name. The variable
#     @command_ret will be set to command prefix that can be used to run the
#     helper. The difference between a helper and a normal command is that
#     a set of environment variable defined using @env_name and @env_value
#     can be passed to the helper script when ever it is executed. These
#     variables can be useful for a cmake helper script to access cmake
#     variables. More environment variables can also be added later using
#     #cmake_helper_add_env. There are also pre-defined variables that
#     will be defined for every script. These are the variables that are
#     currently defined:
#         CMAKE_HELPER_CMAKE_COMMAND: defined to the full path to the
#             cmake command. Use this variable instead of `cmake` to run
#             cmake (e.g. for `cmake -E`).
#         CMAKE_GENERAL_BASE: defined to the cache directory for build files.
#     See also #cmake_helper_add_env.
function(cmake_helper_reg name file command_ret)
  set(bool_name "__cmake_helper_reg_${name}")
  cmake_utils_check_and_set_bool("${bool_name}" registered)
  if(registered)
    message(WARNING
      "Helper script ${name} already registered. Will not register again.")
    return()
  endif()
  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  cmake_utils_get_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER helper_wrapper)

  set(OPTIONS)
  set(SINGLES
    WORKING_DIR)
  set(MULTIPLES
    ENVS)
  set(LISTS)
  cmake_parse_array_default(REG_HELPER 3)
  cmake_array_concat("${REG_HELPER_ENVS}" "${REG_HELPER_UNUSED}")
  cmake_helper_to_cur_file(file "${file}")
  cmake_helper_to_cur_file(REG_HELPER_WORKING_DIR "${REG_HELPER_WORKING_DIR}")

  cmake_utils_reconf_on("${file}")
  cmake_utils_get_unique_name("cmake_helper_${name}_default" default_dir_name)
  cmake_utils_get_unique_name("cmake_helper_${name}_raw" raw_name)
  cmake_array_new(command exec "${file}")
  cmake_array_new(commands "${command}")

  set(env_fname "${helper_cache}/command_${name}_export.sh")
  _cmake_helper_init_script("${env_fname}")

  __cmake_helper_wrap_commands("${env_fname}"
    "${default_dir_name}" "${commands}" "${REG_HELPER_WORKING_DIR}"
    "" "" "" "${REG_HELPER_ENVS}")
  __cmake_helper_wrap_commands("${env_fname}"
    "${raw_name}" "${commands}" "" "" "" "" "")

  _cmake_helper_wrap_shell(wrapper_file "${helper_wrapper}"
    "--run-command" "${default_dir_name}")
  _cmake_helper_wrap_shell(wrapper_file_raw "${helper_wrapper}"
    "--run-command" "${raw_name}")

  cmake_utils_set_global("_cmake_helper_${name}_script" "${file}")
  cmake_utils_set_global("_cmake_helper_${name}_working_dir"
    "${REG_HELPER_WORKING_DIR}")
  cmake_utils_set_global("_cmake_helper_${name}_wrapper_raw"
    "${wrapper_file_raw}")
  cmake_utils_set_global("_cmake_helper_${name}_wrapper" "${wrapper_file}")
  # cmake_utils_set_global("_cmake_helper_${name}_setting_raw" "${raw_name}")
  # cmake_utils_set_global("_cmake_helper_${name}_setting" "${default_dir_name}")
  set("${command_ret}" "${wrapper_file}" PARENT_SCOPE)
  cmake_utils_reconf_on("${env_fname}")
endfunction()

macro(_cmake_helper_save_commands name save_commands_list)
  cmake_array_set_global("_cmake_helper_save_commands_${name}"
    "${save_commands_list}" 2)
endmacro()

macro(_cmake_helper_retrieve_commands name save_commands_list)
  cmake_array_get_global("_cmake_helper_save_commands_${name}"
    "${save_commands_list}" 2)
endmacro()

# cmake_helper_wrap_shell(command_ret
#                         (COMMAND: command...)...
#                         [WORKING_DIR: working_dir]
#                         [ID_RET: id_ret]
#                         [INPUT_FILE: input_file]
#                         [OUTPUT_FILE: output_file]
#                         [ERROR_FILE: error_file]
#                         [ENVS: (env_name env_value)...])
#     @command_ret: variable to return the wrapper command used to
#         run the original command.
#     @command: a single shell command to be wrapped,
#
#     This function wrap a shell command and return a wrapper command. All
#     the arguments including the command itself will be quoted properly.
#     See also #cmake_helper_wrap_helper.
function(cmake_helper_wrap_shell command_ret)
  set(OPTIONS)
  set(SINGLES INPUT_FILE OUTPUT_FILE ERROR_FILE WORKING_DIR ID_RET)
  set(MULTIPLES ENVS)
  set(LISTS COMMAND)
  cmake_parse_array_default(WRAP_SHELL 1)
  if("${WRAP_SHELL_UNUSED}.length")
    cmake_array_append("${WRAP_SHELL_COMMAND}" "${WRAP_SHELL_UNUSED}")
  endif()
  cmake_utils_to_abs(WRAP_SHELL_WORKING_DIR WRAP_SHELL_INPUT_FILE
    WRAP_SHELL_OUTPUT_FILE WRAP_SHELL_ERROR_FILE)

  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  cmake_utils_get_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER helper_wrapper)

  cmake_utils_get_unique_name(cmake_helper_wrap_shell wrapper_name)
  set(env_fname "${helper_cache}/command_${wrapper_name}_export.sh")
  _cmake_helper_init_script("${env_fname}")
  __cmake_helper_wrap_commands("${env_fname}" "${wrapper_name}"
    "${WRAP_SHELL_COMMAND}" "${WRAP_SHELL_WORKING_DIR}"
    "${WRAP_SHELL_INPUT_FILE}" "${WRAP_SHELL_OUTPUT_FILE}"
    "${WRAP_SHELL_ERROR_FILE}" "${WRAP_SHELL_ENVS}")
  _cmake_helper_save_commands("${wrapper_name}" "${WRAP_SHELL_COMMAND}")
  _cmake_helper_wrap_shell(wrapper_file "${helper_wrapper}" "--run-command"
    "${wrapper_name}")
  set("${command_ret}" "${wrapper_file}" PARENT_SCOPE)
  if(NOT "x${WRAP_SHELL_ID_RET}x" STREQUAL xx)
    set("${WRAP_SHELL_ID_RET}" "${wrapper_name}" PARENT_SCOPE)
  endif()
  cmake_utils_reconf_on("${env_fname}")
endfunction()

# cmake_helper_wrap_helper(name command_ret [args...]
#                          (ARGS: args...)...
#                          [WORKING_DIR: working_dir]
#                          [INPUT_FILE: input_file]
#                          [OUTPUT_FILE: output_file]
#                          [ERROR_FILE: error_file]
#                          [ENVS: (env_name env_value)...])
#     @name: name of the helper.
#     @command_ret: variable to return the wrapper command used to
#         run the original command.
#     @args: arguments passed to the helper script.
#
#     This function wrap a helper command and return a wrapper command. All
#     the arguments will be quoted properly. See also #cmake_helper_wrap_shell.
function(cmake_helper_wrap_helper name command_ret)
  set(bool_name "__cmake_helper_reg_${name}")
  cmake_utils_get_bool("${bool_name}" registered)
  if(NOT registered)
    message(FATAL_ERROR
      "Helper script ${name} have not been registered yet.")
    return()
  endif()

  set(OPTIONS)
  set(SINGLES INPUT_FILE OUTPUT_FILE ERROR_FILE WORKING_DIR ID_RET)
  set(MULTIPLES ENVS)
  set(LISTS ARGS)
  cmake_parse_array_default(WRAP_HELPER 2)
  cmake_utils_to_abs(WRAP_HELPER_WORKING_DIR WRAP_HELPER_INPUT_FILE
    WRAP_HELPER_OUTPUT_FILE WRAP_HELPER_ERROR_FILE)
  if(DEFINED "${WRAP_HELPER_UNUSED}.length" AND
      "${${WRAP_HELPER_UNUSED}.length}")
    cmake_array_append("${WRAP_HELPER_ARGS}" "${WRAP_HELPER_UNUSED}")
  endif()

  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  cmake_utils_get_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER helper_wrapper)

  if("x${WRAP_HELPER_WORKING_DIR}x" STREQUAL "xx")
    cmake_utils_get_global("_cmake_helper_${name}_wrapper" wrapper_file)
  else()
    cmake_utils_get_global("_cmake_helper_${name}_wrapper_raw" wrapper_file)
  endif()
  cmake_array_new(command_prefix "${wrapper_file}")

  cmake_array_new(commands)
  macro(__cmake_helper_wrap_helper_foreach)
    foreach(array_name "")
      cmake_array_copy("${command_prefix}" array_name)
      cmake_array_concat("${array_name}" "${helper_arg}")
      cmake_array_append("${commands}" "${array_name}")
    endforeach()
  endmacro()
  cmake_array_foreach(helper_arg __cmake_helper_wrap_helper_foreach
    "${WRAP_HELPER_ARGS}")

  cmake_utils_get_unique_name(cmake_helper_wrap_helper wrapper_name)
  set(env_fname "${helper_cache}/command_${wrapper_name}_export.sh")
  _cmake_helper_init_script("${env_fname}")
  __cmake_helper_wrap_commands("${env_fname}" "${wrapper_name}"
    "${commands}" "${WRAP_HELPER_WORKING_DIR}" "${WRAP_HELPER_INPUT_FILE}"
    "${WRAP_HELPER_OUTPUT_FILE}" "${WRAP_HELPER_ERROR_FILE}"
    "${WRAP_HELPER_ENVS}")
  _cmake_helper_save_commands("${wrapper_name}" "${commands}")
  _cmake_helper_wrap_shell(wrapper_file "${helper_wrapper}" "--run-command"
    "${wrapper_name}")
  set("${command_ret}" "${wrapper_file}" PARENT_SCOPE)
  if(NOT "x${WRAP_HELPER_ID_RET}x" STREQUAL xx)
    set("${WRAP_HELPER_ID_RET}" "${wrapper_name}" PARENT_SCOPE)
  endif()
  cmake_utils_reconf_on("${env_fname}")
endfunction()

function(_cmake_helper_set_command name commands)
  _cmake_helper_save_commands("${name}" "${commands}")
  cmake_utils_get_global(_CMAKE_HELPER_RUN_HELPER_WRAPPER helper_wrapper)
  execute_process(COMMAND "${helper_wrapper}" --rewrite-cmd-settings "${name}")
  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  set(fname "${helper_cache}/command_${name}_setting.sh")
  __cmake_helper_wrap_commands_write_commands("${fname}" "${name}"
    "${commands}")
  cmake_utils_reconf_on("${fname}")
endfunction()

macro(cmake_helper_get_command name ret_commands)
  _cmake_helper_retrieve_commands("${name}" "${ret_commands}")
endmacro()

function(cmake_helper_add_command name)
  set(OPTIONS)
  set(SINGLES)
  set(MULTIPLES)
  set(LISTS COMMAND)
  cmake_parse_array_default(ADD_COMMAND 1)
  _cmake_helper_retrieve_commands("${name}" retrieved_commands)
  cmake_array_concat("${retrieved_commands}" "${ADD_COMMAND_COMMAND}")
  _cmake_helper_set_command("${name}" "${retrieved_commands}")
endfunction()

function(cmake_helper_set_command name)
  set(OPTIONS)
  set(SINGLES)
  set(MULTIPLES)
  set(LISTS COMMAND)
  cmake_parse_array_default(SET_COMMAND 1)
  _cmake_helper_set_command("${name}" "${SET_COMMAND_COMMAND}")
endfunction()

# cmake_helper_add_env(name [[env_name env_value]...])
#     @name: name of the helper.
#     @env_name: name of environment variable.
#     @env_value: value of environment variable.
#
#     This function add more environment variables for a previously
#     registered helper script @name. See #cmake_helper_reg for more detail.
function(cmake_helper_add_env name)
  cmake_utils_get_global(___CMAKE_HELPER_UTILS_CACHE helper_cache)
  set(env_fname "${helper_cache}/command_${name}_export.sh")
  __cmake_helper_escape_envs(envs_string 1)
  file(APPEND "${env_fname}" "${envs_string}")
  cmake_utils_reconf_on("${env_fname}")
endfunction()
