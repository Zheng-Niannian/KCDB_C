cmake_minimum_required(VERSION 3.27)
project(KVServer C)

set(CMAKE_C_STANDARD 11)

add_executable(KVServer main.c
        data_type.h
        transfer.h
        event_handler.c
        event_handler.h
        transfer.c
        request_processor.c)
