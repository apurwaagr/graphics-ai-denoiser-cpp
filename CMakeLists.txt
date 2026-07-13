cmake_minimum_required(VERSION 3.16)
project(GraphicsAIDenoiserCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build unit tests" ON)

add_library(core
    src/core/renderer.cpp
    src/core/denoiser.cpp
)

target_include_directories(core PUBLIC src)

add_executable(graphics_ai_demo src/main.cpp)
target_link_libraries(graphics_ai_demo PRIVATE core)

if(BUILD_TESTING)
    include(CTest)
    enable_testing()

    add_executable(test_denoiser tests/test_denoiser.cpp)
    target_link_libraries(test_denoiser PRIVATE core)
    add_test(NAME denoiser_test COMMAND test_denoiser)
endif()
