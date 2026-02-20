add_executable(myexe
               src1.c src2.c src3.c)
add_library(mylib STATIC
            file.c)
add_custom_target(target
                  COMMAND
                  echo)
