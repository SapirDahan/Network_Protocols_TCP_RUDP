cmake_minimum_required(VERSION 3.27)
project(Networks_Project3 C)

set(CMAKE_C_STANDARD 11)

add_executable(Networks_Project3 main.c
        TCP_Receiver.c
        TCP_Sender.c)
