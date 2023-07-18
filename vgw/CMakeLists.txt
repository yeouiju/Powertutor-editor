cmake_minimum_required(VERSION 3.0.0)
project(virtualgw VERSION 0.1.0 LANGUAGES C)

# include(CTest)
# enable_testing()

add_executable(gw1 vgateway/router_left.c)
target_compile_options(gw1 PRIVATE -Wall -Werror)

add_executable(gw2 vgateway/router_right.c)
target_compile_options(gw2 PRIVATE -Wall -Werror)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
