execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${TEST_DIR}/.cmake_format" ".cmake_format" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "copy .cmake_format failed")
endif()

execute_process(COMMAND "${CMAKEF_EXE}" "--dump-config" OUTPUT_FILE "temp_dump.yml" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "cmakefmt failed")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${TEST_DIR}/expected.yml" "temp_dump.yml" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "compare failed")
endif()
