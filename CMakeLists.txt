cmake_minimum_required(VERSION 3.16)
project(discrete_logarithm_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Preserve the four original lab programs as independent executables.
add_executable(primitive_root src/primitive_root/main.cpp)
add_executable(diffie_hellman src/diffie_hellman/main.cpp)
add_executable(elgamal_decrypt src/elgamal_decrypt/main.cpp)
add_executable(elgamal_verify src/elgamal_verify/main.cpp)
