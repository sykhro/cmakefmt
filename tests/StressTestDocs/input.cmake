add_custom_command(
    OUTPUT foo.cxx
    COMMAND genFromTable -i table.csv -case foo -o foo.cxx
    DEPENDS table.csv # file-level dependency
            generate_table_csv # target-level dependency
    VERBATIM
)

add_custom_command(
    OUTPUT "out-$<CONFIG>.c"
    COMMAND someTool -i ${CMAKE_CURRENT_SOURCE_DIR}/in.txt -o "out-$<CONFIG>.c" -c "$<CONFIG>"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/in.txt
    VERBATIM
)

install(TARGETS myExe mySharedLib myStaticLib
        RUNTIME # Following options apply to runtime artifacts.
        COMPONENT Runtime
        LIBRARY # Following options apply to library artifacts.
        COMPONENT Runtime
        NAMELINK_COMPONENT Development
        ARCHIVE # Following options apply to archive artifacts.
        COMPONENT Development
        DESTINATION lib/static
        FILE_SET HEADERS # Following options apply to file set HEADERS.
        COMPONENT Development
)

install(TARGETS myExe
        CONFIGURATIONS Debug
        RUNTIME
        DESTINATION Debug/bin
)

install(TARGETS myExe
        CONFIGURATIONS Release
        RUNTIME
        DESTINATION Release/bin
)
