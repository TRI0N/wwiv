# CMake for WWIV 5
include_directories(..)

set(NETWORK_MAIN network1.cpp)

add_executable(network1 ${NETWORK_MAIN})
target_link_libraries(network1 networkb_lib core sdk)

