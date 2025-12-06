cmake_minimum_required(VERSION 3.10.0)
project(insert-cheesy-bread)

# Make sure developers do not run cmake in the main project directory, to keep
# build artifacts from becoming clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Turn on debug builds if we are building in a devel shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a sub directory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system log log_setup regex filesystem thread date_time)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

add_library(config_parser_lib src/config_parser.cc)
add_library(session_lib src/session.cc)
add_library(server_lib src/server.cc)
add_library(handler_factory_lib src/handler_factory.cc)
target_link_libraries(handler_factory_lib request_handlers_lib entity_store_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
add_library(handler_registry_lib src/handler_registry.cc)
target_link_libraries(handler_registry_lib handler_factory_lib request_handlers_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
add_library(server_main_utils_lib src/server_main_utils.cc)
add_library(request_dispatcher_lib src/request_dispatcher.cc)
add_library(response_builder_lib src/response_builder.cc)
target_link_libraries(response_builder_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
add_library(request_parser_lib src/request_parser.cc)

# Ensure request_parser_lib links Boost logging and related components since it
# uses BOOST_LOG_TRIVIAL in its implementation
target_link_libraries(request_parser_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)

add_library(request_handlers_lib
    src/request_handler_echo.cc
    src/request_handler_file.cc
    src/request_handler_sleep.cc
    src/request_handler_health.cc
    src/request_handler_not_found.cc
    src/request_handler_api.cc
)

add_library(entity_store_lib
    src/entity_store_in_memory.cc
    src/entity_store_persistent.cc
    src/entity_store_validation.cc
)

add_executable(server_main src/server_main.cc)
target_link_libraries(server_main
    server_main_utils_lib handler_registry_lib server_lib session_lib request_dispatcher_lib config_parser_lib
    Boost::system
)
target_link_libraries(request_dispatcher_lib handler_registry_lib request_handlers_lib request_parser_lib config_parser_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
target_link_libraries(request_handlers_lib response_builder_lib request_parser_lib entity_store_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
target_link_libraries(session_lib request_dispatcher_lib request_handlers_lib response_builder_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
target_link_libraries(server_lib session_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
target_link_libraries(config_parser_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)
target_link_libraries(server_main_utils_lib handler_registry_lib request_handlers_lib request_dispatcher_lib config_parser_lib Boost::log_setup Boost::log Boost::date_time Boost::regex Boost::filesystem Boost::thread Boost::system)

add_executable(config_parser_test tests/config_parser_test.cc)
target_link_libraries(config_parser_test config_parser_lib gtest_main)

add_executable(session_test tests/session_test.cc)
target_link_libraries(session_test session_lib handler_registry_lib request_handlers_lib server_main_utils_lib gtest_main Boost::system)

add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib session_lib handler_registry_lib request_handlers_lib gtest_main gmock Boost::system)

add_executable(server_main_test tests/server_main_test.cc)
target_link_libraries(server_main_test server_main_utils_lib handler_registry_lib request_handlers_lib request_dispatcher_lib config_parser_lib gtest_main)

add_executable(request_parser_test tests/request_parser_test.cc)
target_link_libraries(request_parser_test request_parser_lib gtest_main)

add_executable(response_builder_test tests/response_builder_test.cc)
target_link_libraries(response_builder_test response_builder_lib gtest_main)

add_executable(request_dispatcher_test tests/request_dispatcher_test.cc)
target_link_libraries(request_dispatcher_test request_dispatcher_lib handler_registry_lib request_handlers_lib server_main_utils_lib config_parser_lib gtest_main)

add_executable(not_found_handler_test tests/not_found_handler_test.cc)
target_link_libraries(not_found_handler_test request_handlers_lib response_builder_lib gtest_main)

add_executable(health_handler_test tests/health_handler_test.cc)
target_link_libraries(health_handler_test request_handlers_lib response_builder_lib gtest_main)

add_executable(request_handler_api_test tests/request_handler_api_test.cc)
target_link_libraries(request_handler_api_test request_handlers_lib entity_store_lib gtest_main)

add_executable (entity_store_in_memory_test tests/entity_store_in_memory_test.cc)
target_link_libraries(entity_store_in_memory_test entity_store_lib gtest_main)

gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_test)
gtest_discover_tests(server_test)
gtest_discover_tests(server_main_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(request_parser_test)
gtest_discover_tests(response_builder_test)
gtest_discover_tests(request_dispatcher_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(not_found_handler_test)
gtest_discover_tests(health_handler_test)
gtest_discover_tests(request_handler_api_test)
gtest_discover_tests(entity_store_in_memory_test)

include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(TARGETS server_main 
                                 config_parser_lib 
                                 server_lib 
                                 session_lib 
                                 server_main_utils_lib 
                                 request_dispatcher_lib 
                                 handler_registry_lib
                                 handler_factory_lib
                                 response_builder_lib
                                 request_handlers_lib
                                 request_parser_lib
                                 entity_store_lib
                         TESTS config_parser_test 
                               server_test 
                               session_test 
                               server_main_test
                               request_parser_test
                               response_builder_test
                               request_dispatcher_test
                               not_found_handler_test
                               health_handler_test
                               request_handler_api_test
                               entity_store_in_memory_test)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(integration_test ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.py)
set_tests_properties(integration_test PROPERTIES 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
    TIMEOUT 20
    ENVIRONMENT "SERVER_BINARY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/server_main")

add_test(api_integration_test ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_api_test.py)
set_tests_properties(api_integration_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 30
    ENVIRONMENT "SERVER_BINARY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/server_main")

add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
