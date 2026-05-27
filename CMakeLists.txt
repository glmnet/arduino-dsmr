if(ESP_PLATFORM)
   idf_component_register(INCLUDE_DIRS "src") # header-only
   return()
endif()

cmake_minimum_required(VERSION 3.20)
project(dsmr_parser)
enable_testing()
include(FetchContent)

# doctest
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest
  GIT_TAG v2.5.0)
FetchContent_MakeAvailable(doctest)
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

# mbedtls
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "")
set(ENABLE_TESTING OFF CACHE INTERNAL "")
set(MBEDTLS_FATAL_WARNINGS OFF CACHE INTERNAL "")
FetchContent_Declare(
  mbedtls
  GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls
  GIT_TAG v3.6.5)
FetchContent_MakeAvailable(mbedtls)

# bearssl
FetchContent_Populate(
  bearssl
  GIT_REPOSITORY https://bearssl.org/git/BearSSL
  GIT_TAG v0.6)
file(GLOB_RECURSE bearssl_sources CONFIGURE_DEPENDS "${bearssl_SOURCE_DIR}/src/*.c")
add_library(bearssl STATIC ${bearssl_sources})
target_include_directories(bearssl SYSTEM PUBLIC "${bearssl_SOURCE_DIR}/inc" PRIVATE "${bearssl_SOURCE_DIR}/src")

# cmake_template for warnings and sanitizers
FetchContent_Populate(
  cmake_template
  GIT_REPOSITORY https://github.com/cpp-best-practices/cmake_template
  GIT_TAG a3971f5b4588a5f57a44317a0f7e29827add13f1)
include(${cmake_template_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
include(${cmake_template_SOURCE_DIR}/cmake/Sanitizers.cmake)

# dsmr_parser_test
file(GLOB_RECURSE dsmr_parser_test_src_files CONFIGURE_DEPENDS "src/*.h" "src/*.cpp" "tests/*.h" "tests/*.cpp")
add_executable(dsmr_parser_test ${dsmr_parser_test_src_files})
target_include_directories(dsmr_parser_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(dsmr_parser_test PRIVATE cxx_std_20)
target_link_libraries(dsmr_parser_test PRIVATE mbedtls bearssl doctest::doctest)
target_include_directories(dsmr_parser_test SYSTEM PUBLIC $<TARGET_PROPERTY:mbedtls,INTERFACE_INCLUDE_DIRECTORIES>) # disable warnings for mbedtls headers
target_compile_options(dsmr_parser_test PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>) # enable __VA_OPT__ on MSVC
doctest_discover_tests(dsmr_parser_test)

# enable warnings
add_library(dsmr_parser_test_warnings INTERFACE)
myproject_set_project_warnings(dsmr_parser_test_warnings ON "" "" "" "")
target_link_libraries(dsmr_parser_test PRIVATE dsmr_parser_test_warnings)

# enable sanitizers: address, leak, undefined behaviour
add_library(dsmr_parser_sanitizers INTERFACE)
myproject_enable_sanitizers(dsmr_parser_sanitizers ON ON ON OFF OFF)
target_link_libraries(dsmr_parser_test PUBLIC dsmr_parser_sanitizers)
if(MSVC)
  cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH MSVC_TOOLS_DIR)
  file(GLOB ASAN_RUNTIME_FILES "${MSVC_TOOLS_DIR}/clang_rt.asan_dynamic-*")
  file(COPY ${ASAN_RUNTIME_FILES} DESTINATION "${CMAKE_BINARY_DIR}")
endif()
