cmake_minimum_required(VERSION 3.15)

set(gperf_dir ../thirdparty/gperftools/src)
set(cute_dir ../thirdparty/cute_headers)

add_library(malloc_geiger SHARED malloc_geiger.cpp 
                                 ${gperf_dir}/windows/preamble_patcher.cc 
                                 ${gperf_dir}/windows/preamble_patcher_with_stub.cc
                                 ${gperf_dir}/windows/mini_disassembler.cc
                                 ${gperf_dir}/windows/ia32_opcode_map.cc
                                 ${gperf_dir}/windows/ia32_modrm_map.cc
                                 ${gperf_dir}/base/logging.cc
                                 ${gperf_dir}/windows/port.cc)
target_compile_definitions(malloc_geiger PRIVATE MALLOC_GEIGER_EXPORTS)
target_include_directories(malloc_geiger PRIVATE ${gperf_dir} ${gperf_dir}/windows ${cute_dir})
install(TARGETS malloc_geiger)