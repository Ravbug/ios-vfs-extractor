cmake_minimum_required(VERSION 3.17)
project(ios-vfs-extract)

file(GLOB sources src/*.cpp src/*.hpp)
add_executable(${PROJECT_NAME} ${sources})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC lib/cxxopts/include lib/zlib "${CMAKE_BINARY_DIR}/lib/zlib/")

add_subdirectory(lib/zlib)
target_link_libraries(${PROJECT_NAME} PUBLIC zlibstatic)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC /MP)
endif()