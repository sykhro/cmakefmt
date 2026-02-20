cmake_minimum_required(VERSION 3.10)
project(cmakefmt C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  add_executable(cmakefmt demo/wasm_main.c
                 lexer.c
                 parser.c
                 config.c
                 formatter.c)
  set_target_properties(cmakefmt PROPERTIES OUTPUT_NAME "cmakefmt")
  set_target_properties(cmakefmt PROPERTIES SUFFIX ".js")
  target_link_options(cmakefmt PRIVATE
                      "-sEXPORTED_FUNCTIONS=['_format_cmake_code','_free_string','_get_default_config','_malloc','_free']"
                      "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']"
                      "-sALLOW_MEMORY_GROWTH=1"
                      "-O3")
else()
  add_executable(cmakefmt lexer.c
                 parser.c
                 config.c
                 formatter.c
                 main.c)

  enable_testing()

  function(add_cmakefmt_test name)
    add_test(NAME test_${name}
             COMMAND ${CMAKE_COMMAND}
             -DTEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}
             -DTARGET_FILE=temp_${name}.cmake
             -DCMAKEF_EXE=$<TARGET_FILE:cmakefmt>
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_test.cmake)
  endfunction()

  add_cmakefmt_test(IndentWidth)
  add_cmakefmt_test(UseTab)
  add_cmakefmt_test(SpacesInParens)
  add_cmakefmt_test(SpaceBeforeParens)
  add_cmakefmt_test(AlignArguments)
  add_cmakefmt_test(ClosingParensOnNewLine)
  add_cmakefmt_test(ClosingParensOnNewLineFalse)
  add_cmakefmt_test(KeepShortStatementOnSameLine)
  add_cmakefmt_test(AlwaysBreakAfterFirstArgument)
  add_cmakefmt_test(BreakBeforeKeywordArgument)
  add_cmakefmt_test(StressTestDocs)
  add_cmakefmt_test(AlignOptions)

  add_test(NAME test_DumpConfig
           COMMAND ${CMAKE_COMMAND}
           -DTEST_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/DumpConfig
           -DCMAKEF_EXE=$<TARGET_FILE:cmakefmt>
           -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_dump_test.cmake)
endif()
