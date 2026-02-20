execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${TEST_DIR}/input.cmake" "${TARGET_FILE}" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "copy input.cmake failed")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${TEST_DIR}/.cmake_format" ".cmake_format" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "copy .cmake_format failed")
endif()

execute_process(COMMAND "${CMAKEF_EXE}" "${TARGET_FILE}" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "cmakefmt failed")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${TEST_DIR}/expected.cmake" "${TARGET_FILE}" RESULT_VARIABLE res)
if(res)
    message(FATAL_ERROR "compare failed")
endif()
