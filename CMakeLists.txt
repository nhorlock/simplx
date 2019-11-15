cmake_minimum_required(VERSION 3.0.0)
project(simplx VERSION 0.1.0)

include(CTest)
enable_testing()

include("common_simplx.cmake")
simplx_core_set_cxx_flags()
add_subdirectory("src/simplx_core")
add_subdirectory("tutorial/04_printer_actor_service")
add_subdirectory("tutorial/09_sync_exit")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
