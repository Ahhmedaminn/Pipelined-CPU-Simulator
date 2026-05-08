cmake_minimum_required(VERSION 3.10)
project(Team_8_CA26 C)

set(CMAKE_C_STANDARD 11)

add_executable(main
        main.c
        parser.c
        processor.c
        pipeline.c
        printer.c
        instructions.c
)