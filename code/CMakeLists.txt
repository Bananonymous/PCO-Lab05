cmake_minimum_required(VERSION 3.5)
project(PROG_LAB05 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/multithreadedsort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/quicksort.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils.h
)
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
add_executable(PCO_LAB05_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp ${HEADERS})
add_executable(PCO_LAB05_benchmarks ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/main.cpp ${HEADERS})

target_link_libraries(PCO_LAB05_tests PRIVATE GTest::GTest GTest::Main -lpcosynchro)
target_link_libraries(PCO_LAB05_benchmarks PRIVATE benchmark::benchmark -lpcosynchro)

include(GNUInstallDirs)
install(TARGETS PCO_LAB05_tests
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(TARGETS PCO_LAB05_benchmarks
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
